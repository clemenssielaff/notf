#pragma once

#include <unordered_map>
#include <unordered_set>

#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "notf/app/node_handle.hpp"

NOTF_OPEN_NAMESPACE

// the graph ======================================================================================================== //

class TheGraph final {

    friend struct Accessor<TheGraph, Node>;
    friend struct Accessor<TheGraph, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheGraph);

    /// RAII object to make sure that a frozen scene is *always* unfrozen again
    class NOTF_NODISCARD FreezeGuard {
        friend TheGraph;

        // methods --------------------------------------------------------- //
    private:
        /// Constructor.
        /// @param thread_id    ID of the freezing thread (exposed for testability).
        FreezeGuard(std::thread::id thread_id = std::this_thread::get_id()) : m_thread_id(std::move(thread_id)) {
            if (!TheGraph::_get()._freeze(m_thread_id)) { m_thread_id = std::thread::id(); }
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(FreezeGuard);
        NOTF_NO_HEAP_ALLOCATION(FreezeGuard);

        /// Destructor.
        ~FreezeGuard() {
            if (is_valid()) { TheGraph::_get()._unfreeze(m_thread_id); }
        }

        /// Tests if this FreezeGuard will unfreeze the Graph again, when it goes out of scope.
        bool is_valid() const { return m_thread_id != std::thread::id{}; }

        // fields ---------------------------------------------------------- //
    private:
        /// Id of the freezing thread, if freezing succeeded
        std::thread::id m_thread_id;
    };

private:
    /// Node registry Uuid -> NodeHandle.
    struct NodeRegistry {

        /// The Node with the given Uuid.
        /// @param uuid Uuid of the Node to look up.
        /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
        NodeHandle get_node(Uuid uuid) const;

        /// The number of Nodes in the Registry.
        size_t get_count() const { return m_registry.size(); }

        /// Registers a new Node in the Graph.
        /// @param node                 Node to register.
        /// @throws not_unique_error    If another Node with the same Uuid is already registered.
        void add(NodeHandle node);

        /// Unregisters the Node with the given Uuid.
        /// If the Uuid is not know, this method does nothing.
        void remove(Uuid uuid);

    private:
        /// The registry.
        std::unordered_map<Uuid, NodeHandle> m_registry;

        /// Mutex protecting the registry.
        mutable Mutex m_mutex;
    };

    /// Node name registry std::string <-> NodeHandle.
    struct NodeNameRegistry {

        /// (Re-)Names a Node in the registry.
        /// If another Node with the same name already exists, this method will append the lowest integer postfix that
        /// makes the name unique.
        /// @param node     Node to rename.
        /// @param name     Proposed name of the Node.
        /// @returns        New name of the Node.
        std::string_view set_name(NodeHandle node, const std::string& name);

        /// The Node with the given name.
        /// @param name Name of the Node to look up.
        /// @returns    The requested Handle, is invalid if the name did not identify a Node.
        NodeHandle get_node(const std::string& name) const;

        /// Removes the Node from the registry.
        /// @param uuid Uuid of the Node to remove.
        void remove_node(Uuid uuid);

        /// Mutex protecting the registry.
        RecursiveMutex& get_mutex() { return m_mutex; }

    private:
        /// Manages a static std::string instance to copy into whenever you want to remove a node with a string_view.
        /// @param name_view    string_view of the name of the Node to remove.
        void _remove_name(std::string_view name_view);

    private:
        /// The registry.
        mutable std::unordered_map<std::string, std::pair<Uuid, NodeHandle>> m_name_to_node;
        mutable std::unordered_map<Uuid, std::string_view> m_uuid_to_name;

        /// Mutex protecting the registry.
        mutable RecursiveMutex m_mutex;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Default constructor.
    TheGraph();

    /// The Graph singleton.
    static TheGraph& _get() {
        static TheGraph instance;
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheGraph);

    /// Destructor.
    ~TheGraph();

    // nodes ------------------------------------------------------------------

    /// The current Root Node of this Graph.
    static RootNodeHandle get_root_node();

    /// The Node with the given name.
    /// @param name Name of the Node to look up.
    /// @returns    The requested Handle, is invalid if the name did not identify a Node.
    static NodeHandle get_node(const std::string& name) { return TheGraph::_get().m_node_name_registry.get_node(name); }

    /// The Node with the given Uuid.
    /// @param uuid Uuid of the Node to look up.
    /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
    static NodeHandle get_node(Uuid uuid) { return TheGraph::_get().m_node_registry.get_node(uuid); }

    // freezing ---------------------------------------------------------------

    /// Freeze the Scene, if it is currently unfrozen.
    /// @returns    FreezeGuard that keeps the scene frozen while it is alive, is invalid if freezing did not succeed.
    static FreezeGuard freeze() { return TheGraph::_get()._freeze_guard(); }

    /// Tests whether the Graph singleton is currently frozen.
    static bool is_frozen() noexcept { return TheGraph::_get()._is_frozen(); }

    /// Tests whether the Graph singleton is currently frozen by the given thread.
    static bool is_frozen_by(const std::thread::id thread_id) noexcept {
        return TheGraph::_get()._is_frozen_by(thread_id);
    }

    // global mutexes ---------------------------------------------------------

    /// Mutex used to protect the Graph.
    static RecursiveMutex& get_graph_mutex() { return TheGraph::_get().m_mutex; }

    /// Mutex used to protect the Name Registry.
    static RecursiveMutex& get_name_mutex() { return TheGraph::_get().m_node_name_registry.get_mutex(); }

#ifdef NOTF_DEBUG
    /// Checks if the Graph is locked on the current thread (debug builds only).
    static bool is_locked_by_this_thread() noexcept { return get_graph_mutex().is_locked_by_this_thread(); }
#endif

private:
    /// (Re-)Initializes the Graph.
    /// Is called only once in a regular app, but multiple times during tests to reset the Graph.
    void _initialize();

    /// Freeze the Scene, if it is currently unfrozen.
    /// @param thread_id    Id of the freezing thread (should only be used in tests).
    /// @returns            FreezeGuard that keeps the scene frozen while it is alive, is invalid if freezing did not
    /// succeed.
    FreezeGuard _freeze_guard(const std::thread::id thread_id = std::this_thread::get_id()) {
        return FreezeGuard(std::move(thread_id));
    }

    /// Freezes the Scene if it is not already frozen.
    /// @param thread_id    Id of the freezing thread.
    /// @returns            True iff the Scene was frozen.
    bool _freeze(std::thread::id thread_id);

    /// Unfreezes the Scene again.
    /// @param thread_id    Id of the freezing thread (to ensure that the same thread freezes and unfreezes).
    void _unfreeze(std::thread::id thread_id);

    /// Removes all modified data copies from the Graph - at the point that this method returns, all threads agree on
    /// the complete state of the Graph
    /// @returns    True iff something changed in the Graph since the last synchronization.
    bool _synchronize();

    /// Tests whether this Graph is currently frozen.
    bool _is_frozen() const noexcept { return m_freezing_thread != std::thread::id(); }

    /// Tests whether this Graph is currently frozen by the given thread.
    bool _is_frozen_by(const std::thread::id thread_id) const noexcept { return m_freezing_thread == thread_id; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Mutex used to protect the Graph.
    RecursiveMutex m_mutex;

    /// Node registry Uuid -> NodeHandle.
    NodeRegistry m_node_registry;

    /// Node name registry string -> NodeHandle.
    NodeNameRegistry m_node_name_registry;

    /// The single Root Node in the Graph.
    RootNodePtr m_root_node;

    /// All Nodes that were modified since the last time the Graph was rendered.
    std::unordered_set<NodeHandle> m_dirty_nodes;

    /// Thread that has frozen the Graph (is 0 if the Graph is not frozen).
    std::thread::id m_freezing_thread;
};

// node accessor ==================================================================================================== //

/// Access to selected members of TheGraph.
template<>
class Accessor<TheGraph, Node> {
    friend Node;

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node                 Node to register.
    /// @throws not_unique_error    If another Node with the same Uuid is already registered.
    static void register_node(NodeHandle node) {
        TheGraph::_get().m_node_registry.add(node); // first, because it may fail
        TheGraph::_get().m_dirty_nodes.emplace(std::move(node));
    }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void unregister_node(Uuid uuid) {
        TheGraph::_get().m_node_registry.remove(uuid);
        TheGraph::_get().m_node_name_registry.remove_node(uuid);
    }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param node     Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string_view set_name(NodeHandle node, const std::string& name) {
        return TheGraph::_get().m_node_name_registry.set_name(std::move(node), name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void mark_dirty(NodeHandle node) { TheGraph::_get().m_dirty_nodes.emplace(std::move(node)); }
};

/// Access to selected members of TheGraph.
template<>
class Accessor<TheGraph, Window> {
    friend Window;

    /// The Root Node of the Graph.
    static RootNodePtr get_root_node() { return TheGraph::_get().m_root_node; }

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node                 Node to register.
    /// @throws not_unique_error    If another Node with the same Uuid is already registered.
    static void register_node(NodeHandle node) {
        TheGraph::_get().m_node_registry.add(node); // first, because it may fail
        TheGraph::_get().m_dirty_nodes.emplace(std::move(node));
    }
};

NOTF_CLOSE_NAMESPACE
