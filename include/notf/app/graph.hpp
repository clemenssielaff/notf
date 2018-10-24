#pragma once

#include <unordered_map>
#include <unordered_set>

#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "./node_handle.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

/// A Root Node type must be derived from AnyRootNode and (a subclass of) Node.
template<class T>
static constexpr const bool is_root_node_v = std::conjunction_v<std::is_base_of<AnyRootNode, T>, //
                                                                std::is_base_of<Node, T>>;

} // namespace detail

// the graph ======================================================================================================== //

class TheGraph final {

    friend struct Accessor<TheGraph, Node>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheGraph);

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

        /// Returns the name of the Node.
        /// @param node                 Handle of the Node in question.
        /// @returns                    Name of the Node or an empty string view.
        /// @throws HandleExpiredError  If the NodeHandle has expired.
        std::string_view get_name(NodeHandle node) const;

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
    static TheGraph& _get()
    {
        static TheGraph instance;
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheGraph);

    /// Destructor.
    ~TheGraph();

    /// (Re-)Initializes the Graph with a new Root Node.
    /// All existing Nodes in the Graph are removed.
    /// Blocks if a frame is currently being rendered.
    template<class T, class = std::enable_if_t<detail::is_root_node_v<T>>>
    static void initialize()
    {
        TheGraph::_get()._initialize_typed<T>();
    }

    /// The current Root Node of this Graph.
    static NodeHandle get_root_node(std::thread::id thread_id = std::this_thread::get_id());

    /// The Node with the given name.
    /// @param name Name of the Node to look up.
    /// @returns    The requested Handle, is invalid if the name did not identify a Node.
    static NodeHandle get_node(const std::string& name) { return TheGraph::_get().m_node_name_registry.get_node(name); }

    /// The Node with the given Uuid.
    /// @param uuid Uuid of the Node to look up.
    /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
    static NodeHandle get_node(Uuid uuid) { return TheGraph::_get().m_node_registry.get_node(uuid); }

    /// Tests whether the Graph singleton is currently frozen.
    static bool is_frozen() noexcept { return TheGraph::_get()._is_frozen(); }

    /// Tests whether the Graph singleton is currently frozen by the given thread.
    static bool is_frozen_by(const std::thread::id thread_id) noexcept
    {
        return TheGraph::_get()._is_frozen_by(thread_id);
    }

    /// Mutex used to protect the Graph.
    static RecursiveMutex& get_graph_mutex() { return TheGraph::_get().m_mutex; }

    /// Mutex used to protect the Name Registry.
    static RecursiveMutex& get_name_mutex() { return TheGraph::_get().m_node_name_registry.get_mutex(); }

#ifdef NOTF_DEBUG
    /// Checks if the Graph is locked on the current thread (debug builds only).
    static bool is_locked_by_this_thread() noexcept { return get_graph_mutex().is_locked_by_this_thread(); }
#endif

private:
    template<class T>
    void _initialize_typed()
    {
        // create and initialize the new root node
        auto new_root_node = std::make_shared<T>();
        auto new_node = std::dynamic_pointer_cast<Node>(new_root_node);
        NOTF_ASSERT(new_node);
        _initialize_untyped(std::move(new_root_node));

        // register it
        m_node_registry.add(new_node);
        m_dirty_nodes.emplace(std::move(new_node));
    }

    /// Continues initialization of the Graph with an unfinalized Root Node.
    void _initialize_untyped(AnyRootNodePtr&& root_node);

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
    AnyRootNodePtr m_root_node;

    /// The new Root Node, if the Grahp was re-initialized while frozen.
    AnyRootNodePtr m_modified_root_node;

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
    static void register_node(NodeHandle node)
    {
        TheGraph::_get().m_node_registry.add(node); // first, because it may fail
        TheGraph::_get().m_dirty_nodes.emplace(std::move(node));
    }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void unregister_node(Uuid uuid)
    {
        TheGraph::_get().m_node_registry.remove(uuid);
        TheGraph::_get().m_node_name_registry.remove_node(uuid);
    }

    /// Returns the name of the Node.
    /// @param node                 Handle of the Node in question.
    /// @returns                    Name of the Node or an empty string view.
    /// @throws HandleExpiredError  If the NodeHandle has expired.
    static std::string_view get_name(NodeHandle node)
    {
        return TheGraph::_get().m_node_name_registry.get_name(std::move(node));
    }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param node     Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string_view set_name(NodeHandle node, const std::string& name)
    {
        return TheGraph::_get().m_node_name_registry.set_name(std::move(node), name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void mark_dirty(NodeHandle node) { TheGraph::_get().m_dirty_nodes.emplace(std::move(node)); }
};

NOTF_CLOSE_NAMESPACE
