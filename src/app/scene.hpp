#pragma once

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "app/path.hpp"
#include "app/properties.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _SceneGraph;
class _SceneGraph_NodeHandle;
template<class>
class _Scene;
class _Scene_NodeHandle;
template<class>
class _SceneNode;
class _NodeHandle;
} // namespace access

// ===================================================================================================================//

class SceneGraph {

    // access ------------------------------------------------------------------------------------------------------- //

    friend class access::_SceneGraph<Window>;
    friend class access::_SceneGraph<Scene>;
    friend class access::_SceneGraph<SceneNode>;
    friend class access::_SceneGraph<EventManager>;
    friend class access::_SceneGraph<RenderManager>;
    friend class access::_SceneGraph_NodeHandle;
#ifdef NOTF_TEST
    friend class access::_SceneGraph<test::Harness>;
#endif

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_SceneGraph<T>;
    using NodeHandleAccess = access::_SceneGraph_NodeHandle;

    /// State of the SceneGraph.
    class State {

        // methods ------------------------------------------------------------
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

        /// Constructor.
        /// @param layers   Layers that make up the State, ordered from front to back.
        State(std::vector<valid_ptr<LayerPtr>>&& layers) : m_layers(std::move(layers)) {}

    public:
        /// Layers that make up the State, ordered from front to back.
        const std::vector<valid_ptr<LayerPtr>> layers() const { return m_layers; }

        // fields -------------------------------------------------------------
    private:
        /// Layers that make up the State, ordered from front to back.
        std::vector<valid_ptr<LayerPtr>> m_layers;
    };
    using StatePtr = std::shared_ptr<State>;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param window   Window owning this SceneGraph.
    SceneGraph(Window& window);

    /// Factory.
    /// @param window   Window owning this SceneGraph.
    static SceneGraphPtr _create(Window& window) { return NOTF_MAKE_SHARED_FROM_PRIVATE(SceneGraph, window); }

public:
    NOTF_NO_COPY_OR_ASSIGN(SceneGraph);

    /// Destructor.
    /// Needed, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneGraph();

    /// @{
    /// Window owning this SceneGraph.
    Window& window() { return m_window; }
    const Window& window() const { return m_window; }
    /// @}

    /// Checks if the SceneGraph currently frozen or not.
    bool is_frozen() const
    {
        NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread());
        return (m_freezing_thread != 0);
    }

    /// Checks if the SceneGraph is currently frozen by a given thread.
    /// @param thread_id    Id of the thread in question.
    bool is_frozen_by(const std::thread::id& thread_id) const
    {
        NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread());
        return (m_freezing_thread == hash(thread_id));
    }

    // state management -------------------------------------------------------

    /// Creates a new SceneGraph::State
    /// @param layers   Layers that make up the new state, ordered from front to back.
    static StatePtr create_state(std::vector<valid_ptr<LayerPtr>> layers)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(State, std::move(layers));
    }

    /// @{
    /// The current State of the SceneGraph.
    StatePtr current_state();
    const StatePtr current_state() const { return const_cast<SceneGraph*>(this)->current_state(); }
    /// @}

    /// Schedule this SceneGraph to switch to a new state.
    /// Generates a StateChangeEvent and pushes it onto the event queue for the Window.
    void enter_state(StatePtr new_state);

    /// The PropertyGraph associated with this SceneGraph (owned by the same Window).
    PropertyGraph& property_graph() const;

private:
    /// Registers a SceneNode as dirty.
    /// @param node     Node to register as dirty.
    void _register_dirty(valid_ptr<SceneNode*> node);

    /// Unregisters a (previously registered) dirty node as being clean again.
    /// If the node wasn't registered as dirty to begin with, nothing changes.
    /// @param node     Node to unregister.
    void _remove_dirty(valid_ptr<SceneNode*> node);

    /// Propagates the event into the scenes.
    /// @param untyped_event
    void _propagate_event(EventPtr&& untyped_event);

    // freezing ---------------------------------------------------------------

    /// Freezes the Scene if it is not already frozen.
    /// @param thread_id    Id of the calling thread.
    /// @returns            True iff the Scene was frozen.
    bool _freeze(const std::thread::id thread_id);

    /// Unfreezes the Scene again.
    /// @param thread_id    Id of the calling thread (for safety reasons).
    void _unfreeze(const std::thread::id thread_id);

    // state changes ----------------------------------------------------------

    /// Enters a given State.
    /// @param state    New state to enter.
    void _enter_state(valid_ptr<StatePtr> state);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window owning this SceneGraph.
    Window& m_window;

    /// Current State of the SceneGraph.
    valid_ptr<StatePtr> m_current_state;

    /// Frozen State of the SceneGraph.
    risky_ptr<StatePtr> m_frozen_state = nullptr;

    /// Nodes that registered themselves as "dirty".
    /// If there is one or more dirty Nodes registered, the Window containing this graph must be re-rendered.
    std::unordered_set<valid_ptr<SceneNode*>, smart_pointer_hash<valid_ptr<SceneNode*>>> m_dirty_nodes;

    /// Non-owning references to all Scenes that are affected by the graph's "frozen" state.
    std::set<std::weak_ptr<Scene>, std::owner_less<std::weak_ptr<Scene>>> m_scenes;

    /// Mutex locked while an event is being processed.
    /// This mutex is also aquired by the RenderManager to freeze and unfreeze the graph in between events.
    Mutex m_event_mutex;

    /// Mutex guarding the scene.
    /// Needs to be recursive, because deleting a NodeHandle will require the use of the hierarchy mutex, in case the
    /// graph is currently frozen. However, the deletion will also delete all of the node's children who *also* need the
    /// hierachy mutex in case the graph is currently frozen. QED.
    mutable RecursiveMutex m_hierarchy_mutex;

    /// Thread that has frozen the SceneGraph (is 0 if the graph is not frozen).
    size_t m_freezing_thread = 0;
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_SceneGraph<Window> {
    friend class notf::Window;

    /// Factory.
    /// @param window   Window owning this SceneGraph.
    static SceneGraphPtr create(Window& window) { return SceneGraph::_create(window); }
};

//-----------------------------------------------------------------------------

template<>
class access::_SceneGraph<Scene> {
    friend class notf::Scene;

    /// Registers a new Scene with the graph.
    /// @param graph    SceneGraph to operate on.
    /// @param scene    Scene to register.
    static void register_scene(SceneGraph& graph, ScenePtr scene)
    {
        NOTF_MUTEX_GUARD(graph.m_hierarchy_mutex);
        graph.m_scenes.emplace(std::move(scene));
    }

    /// Direct access to the Graph's hierachy mutex.
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

//-----------------------------------------------------------------------------

template<>
class access::_SceneGraph<SceneNode> {
    friend class notf::SceneNode;
    friend class notf::RootSceneNode;

    /// Registers a new SceneNode as dirty.
    /// @param graph    SceneGraph to operate on.
    /// @param node     Node to register as dirty.
    static void register_dirty(SceneGraph& graph, valid_ptr<SceneNode*> node) { graph._register_dirty(node); }

    /// Unregisters a (previously registered) dirty node as being clean again.
    /// If the node wasn't registered as dirty to begin with, nothing changes.
    /// @param graph    SceneGraph to operate on.
    /// @param node     Node to unregister.
    static void remove_dirty(SceneGraph& graph, valid_ptr<SceneNode*> node) { graph._remove_dirty(node); }

    /// Direct access to the Graph's hierachy mutex.
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

//-----------------------------------------------------------------------------

template<>
class access::_SceneGraph<EventManager> {
    friend class notf::EventManager;

    /// Propagates the event into the scenes.
    /// @param graph            SceneGraph to operate on.
    /// @param untyped_event    Event to propagate.
    static void propagate_event(SceneGraph& graph, EventPtr&& untyped_event)
    {
        graph._propagate_event(std::move(untyped_event));
    }
};

//-----------------------------------------------------------------------------

template<>
class access::_SceneGraph<RenderManager> {
    friend class notf::RenderManager;

    /// Freezes the Scene if it is not already frozen.
    /// @param graph        SceneGraph to operate on.
    /// @param thread_id    Id of the calling thread.
    /// @returns            True iff the Scene was frozen.
    static bool freeze(SceneGraph& graph, std::thread::id thread_id) { return graph._freeze(std::move(thread_id)); }

    /// Unfreezes the Scene again.
    /// @param graph        SceneGraph to operate on.
    /// @param thread_id    Id of the calling thread (for safety reasons).
    static void unfreeze(SceneGraph& graph, const std::thread::id thread_id) { graph._unfreeze(std::move(thread_id)); }
};

//-----------------------------------------------------------------------------

class access::_SceneGraph_NodeHandle {
    template<class>
    friend struct notf::NodeHandle;

    /// Direct access to the Graph's hierachy mutex.SceneNode
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

// ===================================================================================================================//

class Scene {
    friend class access::_Scene<SceneGraph>;
    friend class access::_Scene<SceneNode>;
    friend class access::_Scene_NodeHandle;
#ifdef NOTF_TEST
    friend class access::_Scene<test::Harness>;
#endif

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class T>
    using Access = access::_Scene<T>;
    using NodeHandleAccess = access::_Scene_NodeHandle;

    /// Exception thrown when the SceneGraph has gone out of scope before a Scene tries to access it.
    NOTF_EXCEPTION_TYPE(no_graph);

    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error);

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `create`.
    class FactoryToken {
        friend class Scene;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

private:
    /// Shared pointer to any SceneNode.
    using SceneNodePtr = std::shared_ptr<SceneNode>;

    /// Container for all child nodes of a SceneNode.
    using NodeContainer = std::vector<valid_ptr<SceneNodePtr>>;

    /// Shared pointer to the root node.
    using RootPtr = valid_ptr<std::shared_ptr<RootSceneNode>>;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Scene);

    /// Constructor.
    /// @param token    Factory token provided by Scene::create.
    /// @param manager  The SceneGraph owning this Scene.
    Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph);

    /// Scene Factory method.
    /// @param graph    SceneGraph containing the Scene.
    /// @param args     Additional arguments for the Scene subclass
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Scene, T>::value>>
    static std::shared_ptr<T> create(const valid_ptr<SceneGraphPtr>& graph, Args... args)
    {
        std::shared_ptr<T> scene = std::make_shared<T>(FactoryToken(), graph, std::forward<Args>(args)...);
        access::_SceneGraph<Scene>::register_scene(*graph, scene);
        return scene;
    }

    /// Destructor.
    virtual ~Scene();

    /// @{
    /// The SceneGraph owning this Scene.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph()
    {
        SceneGraphPtr scene_graph = m_graph.lock();
        if (NOTF_UNLIKELY(!scene_graph)) {
            notf_throw(no_graph, "SceneGraph has been deleted");
        }
        return scene_graph;
    }
    valid_ptr<SceneGraphConstPtr> graph() const { return const_cast<Scene*>(this)->graph(); }
    /// @}

    /// @{
    /// The unique root node of this Scene.
    RootSceneNode& root() { return *m_root; }
    const RootSceneNode& root() const { return *m_root; }
    /// @}

    /// The number of SceneNodes in the Scene including the root node (therefore is always >= 1).
    size_t count_nodes() const;

    /// Removes all nodes (except the root node) from the Scene.
    void clear();

    // event handling ---------------------------------------------------------
private:
    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool _handle_event(Event& event NOTF_UNUSED) { return false; }

    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    virtual void _resize_view(Size2i size) = 0;

    // scene hierarchy --------------------------------------------------------
private:
    /// Finds a delta for the given child container and returns it, if it exists.
    /// @param node         SceneNode whose delta to return.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    risky_ptr<NodeContainer*> _get_delta(valid_ptr<const SceneNode*> node);

    /// Creates a new delta for the given child container.
    /// @param node         SceneNode whose delta to create a delta for.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    void _create_delta(valid_ptr<const SceneNode*> node);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The SceneGraph owning this Scene.
    std::weak_ptr<SceneGraph> m_graph;

    /// Map containing copieds of ChildContainer that were modified while the Scene was frozen.
    std::unordered_map<valid_ptr<const SceneNode*>, NodeContainer, pointer_hash<const SceneNode>> m_deltas;

    /// The singular root node of the scene hierarchy.
    RootPtr m_root;
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_Scene<SceneGraph> {
    friend class notf::SceneGraph;

    /// Handles an untyped event.
    /// @param scene    Scene to operate on.
    /// @returns        True iff the event was handled.
    static bool handle_event(Scene& scene, Event& event) { return scene._handle_event(event); }

    /// Resizes the view on this Scene.
    /// @param scene    Scene to operate on.
    /// @param size     Size of the view in pixels.
    static void resize_view(Scene& scene, Size2i size) { scene._resize_view(std::move(size)); }

    /// Called by the SceneGraph after unfreezing, resolves all deltas in this Scene.
    /// @param scene    Scene to operate on.
    static void clear_delta(Scene& scene);
};

//-----------------------------------------------------------------------------

template<>
class access::_Scene<SceneNode> {
    friend class notf::SceneNode;

    /// Container used to store the children of a SceneNode.
    using NodeContainer = Scene::NodeContainer;

    /// Finds a delta for the given child container and returns it, if it exists.
    /// @param scene        Scene to operate on.
    /// @param node         SceneNode whose delta to return.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    static risky_ptr<NodeContainer*> get_delta(Scene& scene, valid_ptr<const SceneNode*> node)
    {
        return scene._get_delta(node);
    }

    /// Creates a new delta for the given child container.
    /// @param scene        Scene to operate on.
    /// @param node         SceneNode whose delta to create a delta for.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    static void create_delta(Scene& scene, valid_ptr<const SceneNode*> node) { scene._create_delta(node); }
};

// ===================================================================================================================//

class SceneNode : public receive_signals {
    friend class access::_SceneNode<Scene>;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Access types.
    template<class T>
    using Access = access::_SceneNode<T>;

    /// Container used to store the children of a SceneNode.
    using NodeContainer = Scene::Access<SceneNode>::NodeContainer;

    /// Thrown when a node did not have the expected position in the hierarchy.
    using hierarchy_error = Scene::hierarchy_error;

    /// Exception thrown when the SceneNode has gone out of scope (Scene must have been deleted).
    NOTF_EXCEPTION_TYPE(no_node);

    /// Exception thrown when you try to do something that is only allowed to do if the node hasn't been finalized yet.
    NOTF_EXCEPTION_TYPE(node_finalized);

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `_add_child`.
    class FactoryToken {
        friend class SceneNode;
        friend class access::_SceneNode<Scene>;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when this Node changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(SceneNode);

    /// Constructor.
    /// @param token    Factory token provided by Node::_add_child.
    /// @param scene    Scene to manage the node.
    /// @param parent   Parent of this node.
    SceneNode(const FactoryToken&, Scene& scene, valid_ptr<SceneNode*> parent);

    /// Destructor.
    virtual ~SceneNode();

    /// @{
    /// The Scene containing this node.
    Scene& scene() { return m_scene; }
    const Scene& scene() const { return m_scene; }
    /// @}

    /// The SceneGraph containing this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph() const { return m_scene.graph(); }

    /// @{
    /// The parent of this node.
    valid_ptr<SceneNode*> parent() { return m_parent; }
    valid_ptr<const SceneNode*> parent() const { return m_parent; }
    /// @}

    /// The user-defined name of this node, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

    /// Updates the name of this Node.
    /// @param name New name of this Node.
    const std::string& set_name(std::string name);

    /// Registers this Node as being dirty.
    /// A ScenGraph containing at least one dirty SceneNode causes the Window to be redrawn at the next opportunity.
    /// Is virtual, so subclasses may decide to ignore this method, for example if the node is invisible.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    virtual void redraw() { SceneGraph::Access<SceneNode>::register_dirty(*graph(), this); }

    // hierarchy --------------------------------------------------------------

    /// The number of direct children of this SceneNode.
    size_t count_children() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
        return _read_children().size();
    }

    /// The number of all (direct and indirect) descendants of this SceneNode.
    size_t count_descendants() const
    {
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
        size_t result = 0;
        _count_descendants_impl(result);
        return result;
    }

    /// Tests, if this Node is a descendant of the given ancestor.
    /// @param ancestor     Potential ancestor to verify.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    bool has_ancestor(const valid_ptr<const SceneNode*> ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two Nodes.
    /// The root node is always a common ancestor, iff the two nodes are in the same scene.
    /// @param other            Other SceneNode to find the common ancestor for.
    /// @throws hierarchy_error If the two nodes are not in the same scene.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    valid_ptr<SceneNode*> common_ancestor(valid_ptr<SceneNode*> other);
    valid_ptr<const SceneNode*> common_ancestor(valid_ptr<const SceneNode*> other) const
    {
        return const_cast<SceneNode*>(this)->common_ancestor(const_cast<SceneNode*>(other.get()));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this Node that has a specific type (can be empty if none is found).
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T>
    risky_ptr<const T*> first_ancestor() const // TODO: this should return a Handle, right?
    {
        if (!std::is_base_of<SceneNode, T>::value) {
            return nullptr;
        }

        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*m_scene.graph()));

        valid_ptr<const SceneNode*> next = parent();
        while (next != next->parent()) {
            if (const T* result = dynamic_cast<T*>(next)) {
                return result;
            }
            next = next->parent();
        }
        return nullptr;
    }
    template<class T>
    risky_ptr<T*> first_ancestor()
    {
        return const_cast<const SceneNode*>(this)->first_ancestor<T>();
    }
    ///@}

    // z-order ------------------------------------------------------------

    /// Checks if this Node is in front of all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_front() const;

    /// Checks if this Node is behind all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_back() const;

    /// Returns true if this node is stacked anywhere in front of the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_in_front_of(const valid_ptr<SceneNode*> sibling) const;

    /// Returns true if this node is stacked anywhere behind the given sibling.
    /// @param sibling  Sibling node to test against.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    bool is_behind(const valid_ptr<SceneNode*> sibling) const;

    /// Moves this Node in front of all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_front();

    /// Moves this Node behind all of its siblings.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_back();

    /// Moves this Node before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_before(const valid_ptr<SceneNode*> sibling);

    /// Moves this Node behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    void stack_behind(const valid_ptr<SceneNode*> sibling);

protected:
    /// Recursive implementation of `count_descendants`.
    void _count_descendants_impl(size_t& result) const
    {
        NOTF_ASSERT(SceneGraph::Access<SceneNode>::mutex(*graph()).is_locked_by_this_thread());
        result += _read_children().size();
        for (const auto& child : _read_children()) {
            child->_count_descendants_impl(result);
        }
    }

    /// All children of this node, orded from back to front.
    /// Never creates a delta.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    const NodeContainer& _read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a new delta if the scene is frozen.
    /// Note that you will need to hold the SceneGraph hierarchy mutex while calling this method, as well as for the
    /// entire lifetime of the returned reference!
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    NodeContainer& _write_children();

    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    ///             Note that all arguments for the Node base class are supplied automatically by this method.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<SceneNode, T>::value>>
    NodeHandle<T> _add_child(Args&&... args)
    {
        // create the node
        auto child = std::make_shared<T>(FactoryToken(), m_scene, this, std::forward<Args>(args)...);
        child->_finalize();
        auto handle = NodeHandle<T>(child);

        { // store the node as a child
            NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
            _write_children().emplace_back(std::move(child));
        }

        return handle;
    }

    template<class T>
    void _remove_child(const NodeHandle<T>& handle)
    {
        if (!handle.is_valid()) {
            return;
        }
        std::shared_ptr<T> node = std::static_pointer_cast<T>(NodeHandle<T>::Access::get(handle).get());
        NOTF_ASSERT(node);

        { // remove the node from the child container
            NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>::mutex(*graph()));
            NodeContainer& children = _write_children();
            auto it = std::find(children.begin(), children.end(), node);
            if (it != children.end()) {
                children.erase(it);
            }
        }
    }

    /// Removes all children of this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    void _clear_children();

    /// Constructs a new Property on this SceneNode.
    /// @param name         Name of the Property.
    /// @param value        Initial value of the Property (also determines its type unless specified explicitly).
    /// @throws node_finalized      If you call this method from anywhere but the constructor.
    /// @throws Path::not_unique    If there already exists a Property of the same name on this SceneNode.
    /// @throws no_graph            If the SceneGraph of the node has been deleted.
    template<class T>
    PropertyHandler<T>& _create_property(std::string name, T&& value)
    {
        if (_is_finalized()) {
            notf_throw_format(node_finalized,
                              "Cannot create Property \"{}\" (or any new Property) on SceneNode \"{}\", "
                              "or in fact any SceneNode that has already been finalized",
                              name, this->name());
        }
        if (m_properties.count(name)) {
            notf_throw_format(Path::not_unique, "SceneNode \"{}\" already has a Property named \"{}\"", this->name(),
                              name);
        }

        // create the property
        PropertyHandler<T> result
            = PropertyGraph::Access<SceneNode>::create_property(graph()->property_graph(), std::move(value), this);
        auto it = m_properties.emplace(std::make_pair(std::move(name), result.property()));
        NOTF_ASSERT(it.second);

        return result;
    }

private:
    /// Finalizes this SceneNode.
    void _finalize() const { s_unfinalized_nodes.erase(this); }

    /// Whether or not this SceneNode has been finalized or not.
    bool _is_finalized() const { return s_unfinalized_nodes.count(this) == 0; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The scene containing this node.
    Scene& m_scene;

    /// The parent of this node.
    /// Is guaranteed to outlive this node.
    valid_ptr<SceneNode*> const m_parent;

    /// All children of this node, ordered from back to front.
    NodeContainer m_children;

    /// All properties of this node, accessible by their (node-unique) name.
    std::unordered_map<std::string, PropertyGraph::Access<SceneNode>::PropertyHeadPtr> m_properties;

    /// The parent-unique name of this Node.
    std::string m_name;

    /// Only unfinalized nodes can create properties and creating new children in an unfinalized node creates no deltas.
    static thread_local std::set<valid_ptr<const SceneNode*>> s_unfinalized_nodes;
};

// accessors ---------------------------------------------------------------------------------------------------------//

template<>
class access::_SceneNode<Scene> {
    friend class notf::Scene;

    /// Creates a factory Token so the Scene can create its RootNode.
    static SceneNode::FactoryToken create_token() { return notf::SceneNode::FactoryToken{}; }

    /// All children of this node, ordered from back to front.
    /// It is okay to access the child container directly here, because the function is only used by the Scene to create
    /// a new delta NodeContainer.
    /// @param node     SceneNode to operate on.
    static const SceneNode::NodeContainer& children(const SceneNode& node) { return node.m_children; }
};

// ===================================================================================================================//

/// The singular root node of a Scene hierarchy.
class RootSceneNode : public SceneNode {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param token    Factory token provided by the Scene.
    /// @param scene    Scene to manage the node.
    RootSceneNode(const FactoryToken& token, Scene& scene) : SceneNode(token, scene, this) {}

    /// Destructor.
    virtual ~RootSceneNode();

    /// Sets  a new child at the top of the Scene hierarchy (below the root).
    /// @param args Arguments that are forwarded to the constructor of the child.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<class T, class... Args>
    NodeHandle<T> set_child(Args&&... args)
    {
        _clear_children();
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Removes the child of the root node, effectively clearing the Scene.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    void clear() { _clear_children(); }
};

// ===================================================================================================================//

template<class T>
struct NodeHandle {
    static_assert(std::is_base_of<SceneNode, T>::value,
                  "The type wrapped by NodeHandle<T> must be a subclass of SceneNode");

    template<class U, class... Args, typename>
    friend NodeHandle<U> SceneNode::_add_child(Args&&... args);

    friend class access::_NodeHandle;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    using Access = access::_NodeHandle;

    // methods ------------------------------------------------------------
private:
    /// Constructor.
    /// @param node     Handled SceneNode.
    NodeHandle(const std::shared_ptr<SceneNode>& node) : m_node(node) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(NodeHandle);

    /// Default constructor.
    NodeHandle() = default;

    /// Move constructor.
    /// @param other    NodeHandle to move from.
    NodeHandle(NodeHandle&& other) : m_node(std::move(other.m_node)) { other.m_node.reset(); }

    /// Move assignment operator.
    /// @param other    NodeHandle to move from.
    NodeHandle& operator=(NodeHandle&& other)
    {
        m_node = std::move(other.m_node);
        other.m_node.reset();
        return *this;
    }

    /// @{
    /// The managed BaseNode instance correctly typed.
    /// @throws SceneNode::no_node  If the handled SceneNode has been deleted.
    T* operator->()
    {
        std::shared_ptr<SceneNode> raw_node = m_node.lock();
        if (NOTF_UNLIKELY(!raw_node)) {
            notf_throw(SceneNode::no_node, "SceneNode has been deleted");
        }
        return static_cast<T*>(raw_node.get());
    }
    const T* operator->() const { return const_cast<NodeHandle<T>*>(this)->operator->(); }
    /// @}

    /// Checks if the Handle is currently valid.
    bool is_valid() const { return !m_node.expired(); }

    // fields -------------------------------------------------------------
private:
    /// Handled SceneNode.
    std::weak_ptr<SceneNode> m_node;
};

// accessors ---------------------------------------------------------------------------------------------------------//

class access::_NodeHandle {
    friend class notf::SceneNode;

    /// Extracts a shared_ptr from a NodeHandle.
    template<class T>
    static risky_ptr<std::shared_ptr<SceneNode>> get(const NodeHandle<T>& handle)
    {
        return handle.m_node.lock();
    }
};
NOTF_CLOSE_NAMESPACE
