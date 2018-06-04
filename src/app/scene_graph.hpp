#pragma once

#include <unordered_set>

#include "app/node_property.hpp"
#include "app/path.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _SceneGraph;
class _SceneGraph_NodeHandle;
} // namespace access

// ================================================================================================================== //

class SceneGraph {

    // access ------------------------------------------------------------------------------------------------------- //

    friend class access::_SceneGraph<Window>;
    friend class access::_SceneGraph<Scene>;
    friend class access::_SceneGraph<Node>;
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

    /// Map containing non-owning references to all Scenes in this graph by name.
    using SceneMap = std::map<std::string, std::weak_ptr<Scene>>;

    // ------------------------------------------------------------------------

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

    // getter -----------------------------------------------------------------

    /// @{
    /// Window owning this SceneGraph.
    Window& window() { return m_window; }
    const Window& window() const { return m_window; }
    /// @}

    /// Returns a Scene in this graph by name.
    /// @param name Name of the requested Scene.
    /// @returns    Scene with the given name, can be empty.
    risky_ptr<ScenePtr> scene(const std::string& name)
    {
        auto it = m_scenes.find(name);
        if (it == m_scenes.end()) {
            return nullptr;
        }
        return it->second.lock();
    }

    /// Searches for and returns a Property in the SceneGraph.
    /// @param path     Path uniquely identifying a Property in the SceneGraph.
    /// @returns        Handle to the requested NodeProperty.
    ///                 Is empty if the Property doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T>
    PropertyHandle<T> property(const Path& path)
    {
        return PropertyHandle<T>(std::dynamic_pointer_cast<TypedNodePropertyPtr<T>>(_property(path)));
    }

    /// Searches for and returns a Node in the SceneGraph.
    /// @param path     Path uniquely identifying a Node in the SceneGraph.
    /// @returns        Handle to the requested Node.
    ///                 Is empty if the Node doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T>
    NodeHandle<T> node(const Path& path)
    {
        return NodeHandle<T>(std::dynamic_pointer_cast<T>(_node(path)));
    }

    // freezing ---------------------------------------------------------------

    /// Checks if the SceneGraph currently frozen or not.
    bool is_frozen() const
    {
        // TODO: you must be able to check whether a SceneGraph is frozen without holding the event mutex
        // why did I think this was necessary? Do I need to make the `m_freezing_thread` an atomic?
        // can we get rid of the event mutex swapping in the scene tests now?
        //        NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread());
        return (m_freezing_thread != 0);
    }

    /// Checks if the SceneGraph is currently frozen by a given thread.
    /// @param thread_id    Id of the thread in question.
    bool is_frozen_by(const std::thread::id& thread_id) const
    {
        //        NOTF_ASSERT(m_event_mutex.is_locked_by_this_thread());
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

private:
    /// Registers a Node as dirty.
    /// @param node     Node to register as dirty.
    void _register_dirty(valid_ptr<Node*> node);

    /// Unregisters a (previously registered) dirty node as being clean again.
    /// If the node wasn't registered as dirty to begin with, nothing changes.
    /// @param node     Node to unregister.
    void _remove_dirty(valid_ptr<Node*> node);

    /// Propagates the event into the scenes.
    /// @param untyped_event
    void _propagate_event(EventPtr&& untyped_event);

    // getter -----------------------------------------------------------------

    /// Private and untyped implementation of `property` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Property.
    /// @returns        Handle to the requested NodeProperty. Is empty if the Property doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Property.
    NodePropertyPtr _property(const Path& path);

    /// Private and untyped implementation of `node` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        Handle to the requested Node. Is empty if the Node doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    NodePtr _node(const Path& path);

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

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Window owning this SceneGraph.
    Window& m_window;

    /// Current State of the SceneGraph.
    valid_ptr<StatePtr> m_current_state;

    /// Frozen State of the SceneGraph.
    risky_ptr<StatePtr> m_frozen_state = nullptr;

    /// Nodes that registered themselves as "dirty".
    /// If there is one or more dirty Nodes registered, the Window containing this graph must be re-rendered.
    std::unordered_set<valid_ptr<Node*>, pointer_hash<valid_ptr<Node*>>> m_dirty_nodes;

    /// All Scenes of this SceneGraph by name.
    SceneMap m_scenes;

    /// Mutex locked while an event is being processed.
    /// This mutex is also aquired by the RenderManager to freeze and unfreeze the graph in between events.
    Mutex m_event_mutex;

    /// Mutex guarding the scene.
    /// Needs to be recursive, because deleting a NodeHandle will require the use of the hierarchy mutex, in case
    /// the graph is currently frozen. However, the deletion will also delete all of the node's children who *also* need
    /// the hierachy mutex in case the graph is currently frozen. QED.
    mutable RecursiveMutex m_hierarchy_mutex;

    /// Thread that has frozen the SceneGraph (is 0 if the graph is not frozen).
    size_t m_freezing_thread = 0;
};

// accessors -------------------------------------------------------------------------------------------------------- //

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

    /// Reserves a name for a Scene by registering an expired weak_ptr with the given name.
    /// It is expected that the Scene factory method registers an actual weak_ptr to a Scene with the same name shortly
    /// after. Signals success with its return boolean.
    /// @param graph    SceneGraph to operate on.
    /// @param name     Scene name to reserve.
    /// @returns        Pair <iterator, bool> where iterator points to an element in the SceneMap with the given name
    ///                 and the boolean flag indicates whether the reservation was successfull or not.
    static auto reserve_scene_name(SceneGraph& graph, std::string name)
    {
        NOTF_ASSERT(graph.m_hierarchy_mutex.is_locked_by_this_thread());
        return graph.m_scenes.emplace(std::move(name), std::weak_ptr<Scene>{});
    }

    /// Registers a new Scene with the graph.
    /// @param graph    SceneGraph to operate on.
    /// @param scene    Scene to register.
    static void register_scene(SceneGraph& graph, ScenePtr scene);

    /// Direct access to the Graph's hierachy mutex.
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

//-----------------------------------------------------------------------------

template<>
class access::_SceneGraph<Node> {
    friend class notf::Node;
    friend class notf::RootNode;

    /// Registers a new Node as dirty.
    /// @param graph    SceneGraph to operate on.
    /// @param node     Node to register as dirty.
    static void register_dirty(SceneGraph& graph, valid_ptr<Node*> node) { graph._register_dirty(node); }

    /// Unregisters a (previously registered) dirty node as being clean again.
    /// If the node wasn't registered as dirty to begin with, nothing changes.
    /// @param graph    SceneGraph to operate on.
    /// @param node     Node to unregister.
    static void remove_dirty(SceneGraph& graph, valid_ptr<Node*> node) { graph._remove_dirty(node); }

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
    // TODO: freeze guard?
};

//-----------------------------------------------------------------------------

class access::_SceneGraph_NodeHandle {
    template<class>
    friend struct notf::NodeHandle;

    /// Direct access to the Graph's hierachy mutex.Node
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

NOTF_CLOSE_NAMESPACE