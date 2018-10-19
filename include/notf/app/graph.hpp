#pragma once

#include <unordered_map>
#include <unordered_set>

#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "./node_handle.hpp"

NOTF_OPEN_NAMESPACE

// the graph ======================================================================================================== //

class TheGraph {

    friend struct Accessor<TheGraph, Node>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    template<class T>
    using AccessFor = Accessor<TheGraph, T>;

private:
    /// Node registry Uuid -> NodeHandle.
    struct NodeRegistry {

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
        Mutex m_mutex;
    };

    /// Node name registry std::string <-> NodeHandle.
    struct NodeNameRegistry {

        /// Returns the name of the Node.
        /// Assigns and returns a random name, if the Node is unnamed.
        /// @param node                 Handle of the Node in question.
        /// @throws HandleExpiredError  If the NodeHandle has expired.
        std::string_view get_name(NodeHandle node);

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
        NodeHandle get_node(const std::string& name);

        /// Removes the Node from the registry.
        /// @param uuid Uuid of the Node to remove.
        void remove_node(Uuid uuid);

    private:
        /// Manages a static std::string instance to copy into whenever you want to remove a node with a string_view.
        /// @param name_view    string_view of the name of the Node to remove.
        void _remove_name(std::string_view name_view);

    private:
        /// The registry.
        std::unordered_map<std::string, std::pair<Uuid, NodeHandle>> m_name_to_node;
        std::unordered_map<Uuid, std::string_view> m_uuid_to_name;

        /// Mutex protecting the registry.
        Mutex m_mutex;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Default constructor.
    TheGraph() = default;

public:
    NOTF_NO_COPY_OR_ASSIGN(TheGraph);

    static TheGraph& get()
    {
        static TheGraph instance;
        return instance;
    }

    static bool is_frozen() noexcept { return s_is_frozen; }
    static bool is_frozen_by(std::thread::id /*thread_id*/) noexcept { return s_is_frozen; }

    /// Mutex used to protect the Graph.
    /// Is exposed as is, so that everyone can lock it locally.
    static RecursiveMutex& get_graph_mutex() { return s_graph_mutex; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    static inline bool s_is_frozen = false;

    /// Mutex used to protect the Graph.
    static inline RecursiveMutex s_graph_mutex;

    /// Node registry Uuid -> NodeHandle.
    static inline NodeRegistry s_node_registry;

    /// Node name registry string -> NodeHandle.
    static inline NodeNameRegistry s_node_name_registry;

    /// All Nodes that were modified since the last time the Graph was rendered.
    static inline std::unordered_set<NodeHandle> s_dirty_nodes;
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
        TheGraph::s_node_registry.add(node); // first, because it may fail
        TheGraph::s_dirty_nodes.emplace(std::move(node));
    }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void unregister_node(Uuid uuid)
    {
        TheGraph::s_node_registry.remove(uuid);
        TheGraph::s_node_name_registry.remove_node(uuid);
    }

    /// Returns the name of the Node.
    /// Assigns and returns a random name, if the Node is unnamed.
    /// @param node                 Handle of the Node in question.
    /// @throws HandleExpiredError  If the NodeHandle has expired.
    static std::string_view get_name(NodeHandle node)
    {
        return TheGraph::s_node_name_registry.get_name(std::move(node));
    }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param node     Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string_view set_name(NodeHandle node, const std::string& name)
    {
        return TheGraph::s_node_name_registry.set_name(std::move(node), name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void mark_dirty(NodeHandle node) { TheGraph::s_dirty_nodes.emplace(std::move(node)); }
};

NOTF_CLOSE_NAMESPACE
