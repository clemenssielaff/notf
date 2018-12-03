#pragma once

#include <unordered_map>
#include <unordered_set>

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "notf/app/node_handle.hpp"

NOTF_OPEN_NAMESPACE

// graph ============================================================================================================ //

namespace detail {

class Graph {

    friend TheGraph;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Graph);

    /// RAII object to make sure that a frozen scene is *always* unfrozen again
    class NOTF_NODISCARD FreezeGuard {
        friend Graph;

        // methods --------------------------------------------------------- //
    private:
        /// Constructor.
        /// @param thread_id    ID of the freezing thread (exposed for testability).
        FreezeGuard(std::thread::id thread_id = std::this_thread::get_id());

    public:
        NOTF_NO_COPY_OR_ASSIGN(FreezeGuard);
        NOTF_NO_HEAP_ALLOCATION(FreezeGuard);

        /// Destructor.
        ~FreezeGuard();

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

        // methods --------------------------------------------------------- //
    public:
        /// The Node with the given Uuid.
        /// @param uuid Uuid of the Node to look up.
        /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
        NodeHandle get_node(Uuid uuid) const;

        /// The number of Nodes in the Registry.
        size_t get_count() const { return m_registry.size(); }

        /// Registers a new Node in the Graph.
        /// @param node               Node to register.
        /// @throws NotUniqueError    If another Node with the same Uuid is already registered.
        void add(NodePtr node);

        /// Unregisters the Node with the given Uuid.
        /// If the Uuid is not know, this method does nothing.
        void remove(Uuid uuid);

        // fields ---------------------------------------------------------- //
    private:
        /// The registry.
        std::unordered_map<Uuid, NodeHandle> m_registry;

        /// Mutex protecting the registry.
        mutable Mutex m_mutex;
    };

    /// Node name registry std::string <-> NodeHandle.
    struct NodeNameRegistry {

        // methods --------------------------------------------------------- //
    public:
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

        // fields ---------------------------------------------------------- //
    private:
        /// The registry.
        mutable std::unordered_map<std::string, std::pair<Uuid, NodeHandle>> m_name_to_node;
        mutable std::unordered_map<Uuid, std::string_view> m_uuid_to_name;

        /// Mutex protecting the registry.
        mutable RecursiveMutex m_mutex;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Graph);

    /// Default Constructor.
    Graph();

    /// Destructor.
    ~Graph();

    // nodes ------------------------------------------------------------------

    /// The current Root Node of this Graph.
    RootNodeHandle get_root_node();

    /// The Node with the given name.
    /// @param name Name of the Node to look up.
    /// @returns    The requested Handle, is invalid if the name did not identify a Node.
    NodeHandle get_node(const std::string& name) { return m_node_name_registry.get_node(name); }

    /// The Node with the given Uuid.
    /// @param uuid Uuid of the Node to look up.
    /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
    NodeHandle get_node(Uuid uuid) { return m_node_registry.get_node(uuid); }

    // freezing ---------------------------------------------------------------

    /// Freeze the Scene, if it is currently unfrozen.
    /// @returns    FreezeGuard that keeps the scene frozen while it is alive, is invalid if freezing did not succeed.
    FreezeGuard freeze() { return _freeze_guard(); }

    /// Tests whether this Graph is currently frozen.
    bool is_frozen() const noexcept { return m_freezing_thread != std::thread::id(); }

    /// Tests whether this Graph is currently frozen by the given thread.
    bool is_frozen_by(const std::thread::id thread_id) const noexcept { return m_freezing_thread == thread_id; }

    // global mutexes ---------------------------------------------------------

    /// Mutex used to protect the Graph.
    RecursiveMutex& get_graph_mutex() { return m_mutex; }

    /// Mutex used to protect the Name Registry.
    RecursiveMutex& get_name_mutex() { return m_node_name_registry.get_mutex(); }
#ifdef NOTF_DEBUG
    /// Checks if the Graph is locked on the current thread (debug builds only).
    bool is_locked_by_this_thread() noexcept { return get_graph_mutex().is_locked_by_this_thread(); }
#endif
private:
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

} // namespace detail

// the graph ======================================================================================================== //

class TheGraph : public ScopedSingleton<detail::Graph> {

    friend Accessor<TheGraph, Node>;
    friend Accessor<TheGraph, Window>;
    friend Accessor<TheGraph, detail::Application>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheGraph);

    // methods --------------------------------------------------------------------------------- //
public:
    using ScopedSingleton<detail::Graph>::ScopedSingleton;

private:
    NOTF_CREATE_SMART_FACTORIES(TheGraph);

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node             Node to register.
    /// @throws NotUniqueError  If another Node with the same Uuid is already registered.
    static void _register_node(NodePtr node) {
        NodeHandle handle = node;
        _get().m_node_registry.add(std::move(node)); // first, because it may fail
        _get().m_dirty_nodes.emplace(std::move(handle));
    }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void _unregister_node(Uuid uuid) {
        if (_get_state() == State::RUNNING) { // ignore unregisters during shutdown
            _get().m_node_registry.remove(uuid);
            _get().m_node_name_registry.remove_node(uuid);
        }
    }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param node     Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string_view _set_name(NodeHandle node, const std::string& name) {
        return _get().m_node_name_registry.set_name(std::move(node), name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void _mark_dirty(NodeHandle node) { _get().m_dirty_nodes.emplace(std::move(node)); }

    /// The Root Node of the Graph as `shared_ptr`.
    static RootNodePtr _get_root_node_ptr() { return _get().m_root_node; }
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheGraph, Node> {
    friend Node;

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node             Node to register.
    /// @throws NotUniqueError  If another Node with the same Uuid is already registered.
    static void register_node(NodePtr node) { TheGraph()._register_node(std::move(node)); }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void unregister_node(Uuid uuid) { TheGraph()._unregister_node(std::move(uuid)); }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param node     Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string_view set_name(NodeHandle node, const std::string& name) {
        return TheGraph()._set_name(std::move(node), name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void mark_dirty(NodeHandle node) { TheGraph()._mark_dirty(std::move(node)); }
};

template<>
class Accessor<TheGraph, Window> {
    friend Window;

    /// The Root Node of the Graph as `shared_ptr`.
    static RootNodePtr get_root_node_ptr() { return TheGraph()._get_root_node_ptr(); }

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node             Node to register.
    /// @throws NotUniqueError  If another Node with the same Uuid is already registered.
    static void register_node(NodePtr node) { TheGraph()._register_node(std::move(node)); }
};

template<>
class Accessor<TheGraph, detail::Application> {
    friend detail::Application;

    /// Creates the ScopedSingleton holder instance of TheGraph.
    template<class... Args>
    static auto create(Args... args) {
        return TheGraph::_create_unique(TheGraph::Holder{}, std::forward<Args>(args)...);
    }
};

NOTF_CLOSE_NAMESPACE
