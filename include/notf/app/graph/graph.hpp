#pragma once

#include <unordered_map>
#include <unordered_set>

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/bimap.hpp"
#include "notf/common/mutex.hpp"
#include "notf/common/uuid.hpp"

#include "notf/app/graph/node_handle.hpp"

NOTF_OPEN_NAMESPACE

// graph ============================================================================================================ //

namespace detail {

class Graph {

    friend TheGraph;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Graph);

private:
    /// Node registry Uuid -> NodeHandle.
    struct NodeRegistry {

        // methods --------------------------------------------------------- //
    public:
        /// The Node with the given Uuid.
        /// @param uuid     Uuid of the Node to look up.
        /// @returns        The requested Handle, is invalid if the uuid did not identify a Node.
        AnyNodeHandle get_node(Uuid uuid) const;

        /// The number of Nodes in the Registry.
        size_t get_count() const { return m_registry.size(); }

        /// Registers a new Node in the Graph.
        /// @param node     Node to register.
        /// @throws NotUniqueError    If another Node with the same Uuid is already registered.
        void add(AnyNodeHandle node);

        /// Unregisters the Node with the given Uuid.
        /// If the Uuid is not know, this method does nothing.
        void remove(Uuid uuid);

        // names --------------------------------------------------------------

        /// The Node with the given name.
        /// @param name     Name of the Node to look up.
        /// @returns        The requested Handle, is invalid if the name did not identify a Node.
        AnyNodeHandle get_node(const std::string& name) const;

        /// The name of the Node with the given Uuuid.
        /// If the Node does not have an existing name, a default one is created in its place.
        /// The default name is a 4-syllable mnemonic name, based on this Node's Uuid.
        /// Is not guaranteed to be unique, but collisions are unlikely with 100^4 possibilities.
        /// @param uuid     Uuid of the Node to look up.
        /// @returns        The requested name.
        std::string get_name(Uuid uuid);

        /// (Re-)Names a Node in the registry.
        /// If another Node with the same name already exists, this method will append the lowest integer postfix that
        /// makes the name unique.
        /// @param uuid     Uuid of the Node to rename.
        /// @param proposal Proposed name of the Node.
        /// @returns        New name of the Node.
        std::string set_name(Uuid uuid, const std::string& proposal);

        // fields ---------------------------------------------------------- //
    private:
        /// The registry.
        std::unordered_map<Uuid, AnyNodeHandle> m_registry;

        /// Bimap UUID <-> Name.
        Bimap<Uuid, std::string> m_name_register;

        /// Mutex protecting the registry.
        mutable Mutex m_mutex;
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
    AnyNodeHandle get_node(const std::string& name) { return m_node_registry.get_node(name); }

    /// The Node with the given Uuid.
    /// @param uuid Uuid of the Node to look up.
    /// @returns    The requested Handle, is invalid if the uuid did not identify a Node.
    AnyNodeHandle get_node(Uuid uuid) { return m_node_registry.get_node(uuid); }

    /// The name of the Node with the given Uuuid.
    /// @param uuid Uuid of the Node to look up.
    /// @returns    The requested name, is empty if not found.
    std::string get_name(Uuid uuid) { return m_node_registry.get_name(uuid); }

    /// The number of Nodes in the current Graph.
    size_t get_node_count() const { return m_node_registry.get_count(); }

    /// Deletes all Nodes (except the RootNode) from the Graph.
    void clear();

    // synchronization --------------------------------------------------------

    /// Removes all modified data copies from the Graph - at the point that this method returns, all threads agree on
    /// the complete state of the Graph
    /// @returns    List of Windows that need to be redrawn after the synchronization.
    std::vector<AnyNodeHandle> synchronize();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Mutex used to protect the Graph.
    Mutex m_mutex;

    /// Node registry Uuid -> NodeHandle.
    NodeRegistry m_node_registry;

    /// The single Root Node in the Graph.
    RootNodePtr m_root_node;

    /// All Nodes that were modified since the last time the Graph was rendered.
    std::unordered_set<AnyNodeHandle> m_dirty_nodes;
};

} // namespace detail

// the graph ======================================================================================================== //

class TheGraph : public ScopedSingleton<detail::Graph> {

    friend Accessor<TheGraph, AnyNode>;
    friend Accessor<TheGraph, Window>;
    friend Accessor<TheGraph, detail::Application>;
    friend Accessor<TheGraph, detail::RenderManager>;

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
    static void _register_node(AnyNodeHandle node) {
        _get().m_node_registry.add(node); // first, because it may fail
        _get().m_dirty_nodes.emplace(std::move(node));
    }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void _unregister_node(Uuid uuid) {
        if (_get_state() == _State::RUNNING) { // ignore unregisters during shutdown
            _get().m_node_registry.remove(uuid);
        }
    }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param uuid     UUID of the Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string _set_name(const Uuid uuid, const std::string& name) {
        return _get().m_node_registry.set_name(uuid, name);
    }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void _mark_dirty(AnyNodeHandle node) { _get().m_dirty_nodes.emplace(std::move(node)); }

    /// The Root Node of the Graph as `shared_ptr`.
    static RootNodePtr _get_root_node_ptr() { return _get().m_root_node; }

    /// Mutex used to protect the Graph.
    static Mutex& _get_mutex() { return _get().m_mutex; }
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheGraph, AnyNode> {
    friend AnyNode;

    /// Registers a new Node in the Graph.
    /// Automatically marks the Node as being dirty as well.
    /// @param node             Node to register.
    /// @throws NotUniqueError  If another Node with the same Uuid is already registered.
    static void register_node(AnyNodeHandle node) { TheGraph()._register_node(std::move(node)); }

    /// Unregisters the Node with the given Uuid.
    /// If the Uuid is not know, this method does nothing.
    static void unregister_node(Uuid uuid) { TheGraph()._unregister_node(std::move(uuid)); }

    /// (Re-)Names a Node in the registry.
    /// If another Node with the same name already exists, this method will append the lowest integer postfix that
    /// makes the name unique.
    /// @param uuid     UUID of the Node to rename.
    /// @param name     Proposed name of the Node.
    /// @returns        New name of the Node.
    static std::string set_name(const Uuid uuid, const std::string& name) { return TheGraph()._set_name(uuid, name); }

    /// Registers the given Node as dirty (a visible Property was modified since the last frame was drawn).
    /// @param node     Dirty node.
    static void mark_dirty(AnyNodeHandle node) { TheGraph()._mark_dirty(std::move(node)); }
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
    static void register_node(AnyNodeHandle node) { TheGraph()._register_node(std::move(node)); }
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

template<>
class Accessor<TheGraph, detail::RenderManager> {
    friend detail::RenderManager;

    /// Mutex used to protect the Graph.
    static Mutex& get_mutex() { return TheGraph::_get_mutex(); }
};

NOTF_CLOSE_NAMESPACE
