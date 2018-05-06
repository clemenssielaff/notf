#pragma once

#include <algorithm>
#include <atomic>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class SceneGraph {

    template<typename>
    friend struct NodeHandle;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    NOTF_ALLOW_ACCESS_TYPES(Window, Scene, SceneNode, EventManager);

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
    static SceneGraphPtr create(Window& window) { return NOTF_MAKE_SHARED_FROM_PRIVATE(SceneGraph, window); }

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
    bool is_frozen() const { return (m_freezing_thread != 0); }

    /// Checks if the SceneGraph is currently frozen by a given thread.
    /// @param thread_id    Id of the thread in question.
    bool is_frozen_by(const std::thread::id& thread_id) const { return (m_freezing_thread == hash(thread_id)); }

    // state management -------------------------------------------------------

    /// Creates a new SceneGraph::State
    /// @param layers   Layers that make up the new state, ordered from front to back.
    static StatePtr create_state(std::vector<valid_ptr<LayerPtr>> layers)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(State, std::move(layers));
    }

    /// @{
    /// The current State of the SceneGraph.
    StatePtr current_state()
    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);
        return m_current_state;
    }
    const StatePtr current_state() const
    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);
        return m_current_state;
    }
    /// @}

    /// Enters a given State.
    /// @param state    New state to enter.
    void enter_state(valid_ptr<StatePtr> state);

private:
    /// Registers a SceneNode as dirty.
    /// @param node     Node to register as dirty.
    void _register_dirty(valid_ptr<SceneNode*> node);

    /// Propagates the event into the scenes.
    /// @param untyped_event
    void _propagate_event(EventPtr&& untyped_event);

    // freezing ---------------------------------------------------------------

    /// Freezes the Scene if it is not already frozen.
    /// @param thread_id    Id of the calling thread.
    /// @returns            True iff the Scene was frozen.
    bool _freeze(const std::thread::id thread_id);

    /// Unfreezes the Scene again.
    void _unfreeze(const std::thread::id thread_id);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window owning this SceneGraph.
    Window& m_window;

    /// Current State of the SceneGraph.
    valid_ptr<StatePtr> m_current_state;

    /// Nodes that registered themselves as "dirty".
    /// If there is one or more dirty Nodes registered, the Window containing this graph must be re-rendered.
    std::unordered_set<valid_ptr<SceneNode*>> m_dirty_nodes;

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
    std::atomic_size_t m_freezing_thread{0};
};

template<>
class SceneGraph::Access<Window> {
    friend class Window;

    /// Constructor.
    /// @param graph    Graph to access.
    Access(SceneGraph& graph) : m_graph(graph) {}

    /// Factory.
    /// @param window   Window owning this SceneGraph.
    static SceneGraphPtr create(Window& window) { return SceneGraph::create(window); }

    // fields -----------------------------------------------------------------
private:
    /// Graph to access.
    SceneGraph& m_graph;
};

template<>
class SceneGraph::Access<Scene> {
    friend class Scene;

    /// Constructor.
    /// @param graph    Graph to access.
    Access(SceneGraph& graph) : m_graph(graph) {}

    /// Registers a new Scene with the graph.
    /// @param scene    Scene t register.
    void register_scene(ScenePtr scene)
    {
        std::lock_guard<decltype(m_hierarchy_mutex)> lock(m_graph.m_hierarchy_mutex);
        m_graph.m_scenes.emplace(std::move(scene));
    }

    /// Direct access to the Graph's hierachy mutex.
    RecursiveMutex& mutex() { return m_graph.m_hierarchy_mutex; }

    // fields -----------------------------------------------------------------
private:
    /// Graph to access.
    SceneGraph& m_graph;
};

template<>
class SceneGraph::Access<SceneNode> {
    friend class SceneNode;

    /// Constructor.
    /// @param graph    Graph to access.
    Access(SceneGraph& graph) : m_graph(graph) {}

    /// Registers a new SceneNode as dirty.
    /// @param node     Node to register as dirty.
    void register_dirty(valid_ptr<SceneNode*> node) { m_graph._register_dirty(std::move(node)); }

    /// Direct access to the Graph's hierachy mutex.
    RecursiveMutex& mutex() { return m_graph.m_hierarchy_mutex; }

    // fields -----------------------------------------------------------------
private:
    /// Graph to access.
    SceneGraph& m_graph;
};

template<>
class SceneGraph::Access<EventManager> {
    friend class EventManager;

    /// Constructor.
    /// @param graph    Graph to access.
    Access(SceneGraph& graph) : m_graph(graph) {}

    /// Propagates the event into the scenes.
    /// @param untyped_event
    void propagate_event(EventPtr&& untyped_event) { m_graph._propagate_event(std::move(untyped_event)); }

    // fields -----------------------------------------------------------------
private:
    /// Graph to access.
    SceneGraph& m_graph;
};

// ===================================================================================================================//

class Scene {

    friend class SceneNode;

    template<typename>
    friend struct NodeHandle;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ALLOW_ACCESS_TYPES(SceneGraph);

    /// Exception thrown when the SceneGraph has gone out of scope before a Scene tries to access it.
    NOTF_EXCEPTION_TYPE(no_graph);

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `create`.
    class FactoryToken {
        friend class Scene;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

private:
    using ChildContainer = std::vector<valid_ptr<SceneNode*>>;

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
    template<typename T, typename... Args, typename = std::enable_if_t<std::is_base_of<Scene, T>::value>>
    static std::shared_ptr<T> create(const valid_ptr<SceneGraphPtr>& graph, Args... args)
    {
        std::shared_ptr<T> scene = std::make_shared<T>(FactoryToken(), graph, std::forward<Args>(args)...);
        SceneGraph::Access<Scene>(*graph).register_scene(scene);
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
    /// Deletes the given node from the Scene.
    /// @param node     Node to remove.
    void _delete_node(valid_ptr<SceneNode*> node);

    /// Creates the root node.
    RootSceneNode* _create_root();

    /// Called by the SceneGraph after unfreezing, resolves all deltas in this Scene.
    void _resolve_delta();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The SceneGraph owning this Scene.
    std::weak_ptr<SceneGraph> m_graph;

    /// Map owning all nodes in the scene.
    std::unordered_map<valid_ptr<const SceneNode*>, std::unique_ptr<SceneNode>, pointer_hash<SceneNode>> m_nodes;

    /// Mapping from all nodes in the scene to their children.
    /// Is separate from the Node type so the Scene hierarchy can be frozen.
    std::unordered_map<valid_ptr<const ChildContainer*>, std::unique_ptr<ChildContainer>, pointer_hash<ChildContainer>>
        m_child_container;

    /// Map containing copieds of ChildContainer that were modified while the Scene was frozen.
    std::unordered_map<valid_ptr<const ChildContainer*>, std::unique_ptr<ChildContainer>, pointer_hash<ChildContainer>>
        m_deltas;

    /// All nodes that were deleted while the Scene was frozen.
    std::unordered_set<valid_ptr<SceneNode*>> m_deletion_deltas;

    /// The singular root node of the scene hierarchy.
    valid_ptr<RootSceneNode*> m_root;
};

template<>
class Scene::Access<SceneGraph> {
    friend class SceneGraph;

    /// Constructor.
    /// @param scene    Scene to access.
    Access(Scene& scene) : m_scene(scene) {}

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    bool handle_event(Event& event) { return m_scene._handle_event(event); }

    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    void resize_view(Size2i size) { m_scene._resize_view(std::move(size)); }

    /// Called by the SceneGraph after unfreezing, resolves all deltas in this Scene.
    void resolve_delta() { m_scene._resolve_delta(); }

    // fields -----------------------------------------------------------------
private:
    /// Scene to access.
    Scene& m_scene;
};

// ===================================================================================================================//

/// Base of all Nodes in a Scene.
/// Contains a handle to its children and the Scene from which to request them.
/// This allows us to freeze the Scene hierarchy during rendering with the only downside of an additional
/// indirection between the node and its children.
class SceneNode : public receive_signals {

    // types --------------------------------------------------------------
public:
    NOTF_ALLOW_ACCESS_TYPES(Scene);

    /// Container used to store the children of a SceneNode.
    using ChildContainer = Scene::ChildContainer;

protected:
    /// Factory token object to make sure that Node instances can only be created by a call to `_add_child`.
    class FactoryToken {
        friend class SceneNode;
        FactoryToken() {} // not `= default`, otherwise you could construct a Token via `Token{}`.
    };

    //=========================================================================
public:
    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error);

    // signals ------------------------------------------------------------
public:
    /// Emitted when this Node changes its name.
    /// @param The new name.
    Signal<const std::string&> on_name_changed;

    // methods ------------------------------------------------------------
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

    /// @{
    /// The SceneGraph containing this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    valid_ptr<SceneGraphPtr> graph() { return m_scene.graph(); }
    valid_ptr<SceneGraphConstPtr> graph() const { return m_scene.graph(); }
    /// @}

    /// @{
    /// The parent of this node.
    valid_ptr<SceneNode*> parent() { return m_parent; }
    valid_ptr<const SceneNode*> parent() const { return m_parent; }
    /// @}

    /// All children of this node, orded from back to front.
    /// Never creates a delta.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    const ChildContainer& read_children() const;

    /// All children of this node, orded from back to front.
    /// Will create a new delta if the scene is frozen.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    ChildContainer& write_children();

    /// The user-defined name of this node, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

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
    template<typename T>
    risky_ptr<const T*> first_ancestor() const
    {
        if (!std::is_base_of<SceneNode, T>::value) {
            return nullptr;
        }

        // lock the SceneGraph hierarchy
        NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*m_scene.graph()).mutex());

        valid_ptr<const SceneNode*> next = parent();
        while (next != next->parent()) {
            if (const T* result = dynamic_cast<T*>(next)) {
                return result;
            }
            next = next->parent();
        }
        return nullptr;
    }
    template<typename T>
    risky_ptr<T*> first_ancestor()
    {
        return const_cast<const SceneNode*>(this)->first_ancestor<T>();
    }
    ///@}

    /// Updates the name of this Node.
    /// @param name New name of this Node.
    const std::string& set_name(std::string name);

    /// Registers this Node as being dirty.
    /// A ScenGraph containing at least one dirty SceneNode causes the Window to be redrawn at the next opportunity.
    /// Is virtual, so subclasses may decide to ignore this method, for example if the node is invisible.
    /// @throws no_graph        If the SceneGraph of the node has been deleted.
    virtual void redraw() { SceneGraph::Access<SceneNode>(*graph()).register_dirty(this); }

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
    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    ///             Note that all arguments for the Node base class are supplied automatically by this method.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    template<typename T, typename... Args>
    NodeHandle<T> _add_child(Args&&... args)
    {
        std::unique_ptr<SceneNode> new_child
            = std::make_unique<T>(FactoryToken(), m_scene, this, std::forward<Args>(args)...);
        SceneNode* child_handle = new_child.get();
        {
            NOTF_MUTEX_GUARD(SceneGraph::Access<SceneNode>(*graph()).mutex());
            m_scene.m_nodes.emplace(std::make_pair(child_handle, std::move(new_child)));
            write_children().emplace_back(child_handle);
        }
        return NodeHandle<T>(&m_scene, child_handle);
    }

private:
    /// Registers a new node with the scene.
    /// @param scene        Scene to manage the children.
    /// @returns            An new empty vector for the children of this node.
    /// @throws no_graph    If the SceneGraph of the node has been deleted.
    valid_ptr<ChildContainer*> _register_with_scene(Scene& scene);

    // fields -------------------------------------------------------------
private:
    /// The scene containing this node.
    Scene& m_scene;

    /// The parent of this node.
    /// Is guaranteed to outlive this node.
    valid_ptr<SceneNode*> const m_parent;

    /// Handle of a vector of all children of this node, ordered from back to front.
    valid_ptr<ChildContainer*> const m_children;

    /// The user-defined name of this Node, is potentially empty and not guaranteed to be unique if set.
    std::string m_name;
};

template<>
class SceneNode::Access<Scene> {
    friend class Scene;

    /// Creates a factory Token so the Scene can create its RootNode.
    static SceneNode::FactoryToken create_token() { return FactoryToken{}; }
};

// ===================================================================================================================//

/// The singular root node of a Scene hierarchy.
class RootSceneNode : public SceneNode {

    // methods --------------------------------------------------------------------------
public:
    /// Constructor.
    /// @param token    Factory token provided by the Scene.
    /// @param scene    Scene to manage the node.
    RootSceneNode(const FactoryToken& token, Scene& scene) : SceneNode(token, scene, this) {}

    /// Destructor.
    virtual ~RootSceneNode();

    /// Creates and adds a new child to this node.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<typename T, typename... Args>
    NodeHandle<T> add_child(Args&&... args) // TODO: this should be set_child, because it is the root
    {
        return _add_child<T>(std::forward<Args>(args)...);
    }
};

// ===================================================================================================================//

/// A NodeHandle is the user-facing part of a scene node instance.
/// It is what a call to `BaseNode::_add_child` returns and manages the lifetime of the created child.
/// NodeHandle are not copyable or assignable to make sure that the parent node always outlives its child nodes.
/// The handle is templated on the BaseNode subtype that it contains, so the owner has direct access to the node
/// and doesn't manually need to cast.
template<typename T>
struct NodeHandle {
    static_assert(std::is_base_of<SceneNode, T>::value,
                  "The type wrapped by NodeHandle<T> must be a subclass of SceneNode");

    template<typename U, typename... Args>
    friend NodeHandle<U> SceneNode::_add_child(Args&&... args);

    // methods ------------------------------------------------------------
private:
    /// Constructor.
    /// @param scene    Scene to manage the node.
    /// @param node     Managed BaseNode instance.
    NodeHandle(valid_ptr<Scene*> scene, valid_ptr<SceneNode*> node) : m_scene(scene), m_node(node) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(NodeHandle);

    /// Default constructor.
    NodeHandle() = default;

    /// Move constructor.
    /// @param other    NodeHandle to move from.
    NodeHandle(NodeHandle&& other) : m_scene(other.m_scene), m_node(other.m_node)
    {
        other.m_scene = nullptr;
        other.m_node = nullptr;
    }

    /// Move assignment operator.
    /// @param other    NodeHandle to move from.
    NodeHandle& operator=(NodeHandle&& other)
    {
        m_scene = other.m_scene;
        m_node = other.m_node;
        other.m_scene = nullptr;
        other.m_node = nullptr;
        return *this;
    }

    /// Destructor.
    ~NodeHandle()
    {
        if (!m_scene) {
            return;
        }

        SceneGraphPtr scene_graph;
        try {
            scene_graph = m_scene->graph();
        }
        catch (const Scene::no_graph&) {
            return; // TODO: what do we do if a NodeHandle outlives the SceneGraph?
        }

        NOTF_MUTEX_GUARD(scene_graph->m_hierarchy_mutex);

        valid_ptr<SceneNode*> parent = m_node->parent();
        NOTF_ASSERT_MSG(m_scene->m_nodes.count(parent) != 0,
                        "The parent of Node \"{}\" was deleted before its child was! \n"
                        "Have you been storing NodeHandles of child nodes outside their parent?",
                        m_node->name());

        // remove yourself as a child from your parent node
        {
            SceneNode::ChildContainer& siblings = parent->write_children(); // this creates a delta in a frozen scene
            auto child_it = std::find(siblings.begin(), siblings.end(), m_node);
            NOTF_ASSERT_MSG(child_it != siblings.end(), "Cannot remove unknown child node from parent");
            siblings.erase(child_it);
        }

        // if this scene is currently frozen, add a deletion delta
        if (scene_graph->is_frozen()) {
            m_scene->m_deletion_deltas.insert(m_node);
        }

        // if the scene is not frozen, we can remove the node immediately
        else {
            m_scene->_delete_node(m_node);
        }
    }

    /// @{
    /// The managed BaseNode instance correctly typed.
    constexpr T* operator->() { return static_cast<T*>(m_node.get()); }
    constexpr const T* operator->() const { return static_cast<const T*>(m_node.get()); }
    /// @}

    /// @{
    /// The managed BaseNode instance correctly typed.
    T& operator*() { return *static_cast<T*>(m_node.get()); }
    const T& operator*() const { return *static_cast<T*>(m_node.get()); }
    /// @}

    // fields -------------------------------------------------------------
private:
    /// The scene containing this node.
    risky_ptr<Scene*> m_scene = nullptr;

    /// Managed BaseNode instance.
    risky_ptr<SceneNode*> m_node = nullptr;
};

NOTF_CLOSE_NAMESPACE
