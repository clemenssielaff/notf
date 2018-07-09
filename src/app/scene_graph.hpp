#pragma once

#include <unordered_set>

#include "app/node_property.hpp"
#include "app/path.hpp"
#include "common/aabr.hpp"

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
    using SceneMap = std::map<std::string, SceneWeakPtr>;

    /// Layers are managed by shared pointers, so we can keep track of them in the SceneGraph.
    NOTF_DEFINE_SHARED_POINTERS(class, Layer);

    /// Compositions are managed by shared pointers, because they have to be passed around by events etc. and it's just
    /// nicer not to copy them around the place as much.
    NOTF_DEFINE_SHARED_POINTERS(class, Composition);

    /// The SceneGraph offers two classes: LayerPtr and ScenePtr, that live in user-space.
    /// However, when a Window is closed, instances of those classes become invalid and any further access to them will
    /// cause either a hard crash ... or this exception, which is preferable.
    NOTF_EXCEPTION_TYPE(no_window_error);

    // ========================================================================

    /// Layers are screen-axis-aligned quads that are drawn directly into the screen buffer by the SceneGraph.
    /// The contents of a Layer are clipped to its area.
    /// The Layer's Visualizer can query the size of this area using GraphicsContext::render_area().size() when drawing.
    class Layer {
        friend class SceneGraph;

        // methods -------------------------------------------------------------------------------------------------- //
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

        /// Constructor, constructs a full-screen, visible Layer.
        /// @param scene        Scene displayed in this Layer.
        /// @param visualizer   Visualizer that draws the Scene into this Layer.
        Layer(valid_ptr<ScenePtr> scene, valid_ptr<VisualizerPtr> visualizer);

    public:
        NOTF_NO_COPY_OR_ASSIGN(Layer);

        /// Factory, constructs a full-screen, visible Layer.
        /// @param scene        Scene displayed in this Layer.
        /// @param visualizer   Visualizer that draws the Scene into this Layer.
        static LayerPtr create(valid_ptr<ScenePtr> scene, valid_ptr<VisualizerPtr> visualizer);

        /// Whether the Layer is visible or not.
        bool is_visible() const { return m_is_visible; }

        /// Whether the Layer is active or not.
        bool is_active() const { return m_is_active; }

        /// Whether the Layer is fullscreen or not.
        bool is_fullscreen() const { return m_is_fullscreen; }

        /// Area of this Layer when not fullscreen.
        const Aabri& get_area() const { return m_area; }

        /// The Scene displayed in this Layer.
        /// @throws no_window_error     If the Window containing this Layer has been closed.
        Scene& get_scene()
        {
            if (NOTF_LIKELY(m_scene)) {
                return *raw_pointer(m_scene);
            }
            NOTF_THROW(no_window_error, "Cannot get the Scene from a Layer of a closed Window");
        }

        /// Invisible Layers are not drawn on screen.
        /// Note that this method also changes the `active` state of the Layer to the visibility state.
        /// If you want a hidden/active or visible/inactive combo, call `set_active` after this method.
        void set_visible(const bool is_visible)
        {
            m_is_visible = is_visible;
            m_is_active = is_visible;
        }

        /// Inactive Layers do not participate in event propagation.
        void set_active(const bool is_active) { m_is_active = is_active; }

        /// Sets the Layer to either be always drawn fullscreen (no matter the resolution),
        /// or to respect its explicit size and position.
        void set_fullscreen(const bool is_fullscreen) { m_is_fullscreen = is_fullscreen; }

        /// Sets a new area for this Layer to draw into (but does not change its `fullscreen` state).
        void set_area(Aabri area) { m_area = area; }

        /// Draw the Layer.
        /// @throws no_window_error     If the Window containing this Layer has been closed.
        void draw();

        // fields --------------------------------------------------------------------------------------------------- //
    private:
        /// The Scene displayed in this Layer.
        ScenePtr m_scene;

        /// Visualizer that draws the Scene into this Layer.
        VisualizerPtr m_visualizer;

        /// Area of this Layer when not fullscreen.
        Aabri m_area = Aabri::zero();

        /// Layers can be set invisible in which case they are simply not drawn.
        bool m_is_visible = true;

        /// Layers can be active (the default) or inactive, in which case they do not participate in the event
        /// propagation.
        bool m_is_active = true;

        /// Layers can be drawn either fullscreen (no matter the resolution), or in an AABR with explicit size and
        /// position.
        bool m_is_fullscreen = true;
    };

    // ========================================================================

    /// The Composition of the SceneGraph is a simple list of Layer.
    class Composition {

        // methods ------------------------------------------------------------
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

        /// Constructor.
        /// @param layers   Layers that make up the Composition, ordered from front to back.
        Composition(std::vector<valid_ptr<LayerPtr>> layers) : m_layers(std::move(layers)) {}

    public:
        /// Factory.
        /// @param layers   Layers that make up the Composition, ordered from front to back.
        static CompositionPtr create(std::vector<valid_ptr<LayerPtr>> layers)
        {
            return NOTF_MAKE_SHARED_FROM_PRIVATE(Composition, std::move(layers));
        }

        /// Layers that make up the Composition, ordered from front to back.
        const std::vector<valid_ptr<LayerPtr>>& get_layers() const { return m_layers; }

        // fields -------------------------------------------------------------
    private:
        /// Layers that make up the Composition, ordered from front to back.
        std::vector<valid_ptr<LayerPtr>> m_layers;
    };

    // ========================================================================

    /// RAII object to make sure that a frozen scene is ALWAYS unfrozen again
    class NOTF_NODISCARD FreezeGuard {

        friend class SceneGraph;

        /// Constructor.
        /// @param graph        SceneGraph to freeze.
        /// @param thread_id    ID of the freezing thread, uses the calling thread by default. (exposed for testability)
        FreezeGuard(valid_ptr<SceneGraph*> graph, std::thread::id thread_id = std::this_thread::get_id())
            : m_graph(graph), m_thread_id(std::move(thread_id))
        {
            if (!m_graph->_freeze(m_thread_id)) {
                m_graph = nullptr;
            }
        }

    public:
        NOTF_NO_COPY_OR_ASSIGN(FreezeGuard);
        NOTF_NO_HEAP_ALLOCATION(FreezeGuard);

        /// Move constructor.
        /// @param other    FreezeGuard to move from.
        FreezeGuard(FreezeGuard&& other) : m_graph(other.m_graph) { other.m_graph = nullptr; }

        /// Destructor.
        ~FreezeGuard()
        {
            if (m_graph) { // don't unfreeze if this guard tried to double-freeze the scene
                m_graph->_unfreeze(m_thread_id);
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Scene to freeze.
        risky_ptr<SceneGraph*> m_graph;

        /// Id of the freezing thread.
        const std::thread::id m_thread_id;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param window   Window owning this SceneGraph.
    SceneGraph(valid_ptr<WindowPtr> window);

    /// Factory.
    /// @param window   Window owning this SceneGraph.
    static SceneGraphPtr _create(valid_ptr<WindowPtr> window)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(SceneGraph, std::move(window));
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(SceneGraph);

    /// Destructor.
    /// Needed, because otherwise we'd have to include types contained in member unique_ptrs in the header.
    ~SceneGraph();

    // getter -----------------------------------------------------------------

    /// Window owning this SceneGraph. Is empty if the Window was already closed.
    risky_ptr<WindowPtr> get_window() const { return m_window.lock(); }

    /// Returns a Scene in this graph by name.
    /// @param name Name of the requested Scene.
    /// @returns    Scene with the given name, can be empty.
    risky_ptr<ScenePtr> get_scene(const std::string& name)
    {
        auto it = m_scenes.find(name);
        if (it == m_scenes.end()) {
            return nullptr;
        }
        return it->second.lock();
    }

    /// @{
    /// Searches for and returns a Property in the SceneGraph.
    /// @param path     Path uniquely identifying a Property in the SceneGraph.
    /// @returns        Handle to the requested NodeProperty.
    ///                 Is empty if the Property doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T>
    PropertyHandle<T> get_property(const Path& path)
    {
        return PropertyHandle<T>(std::dynamic_pointer_cast<TypedNodeProperty<T>>(_get_property(path)));
    }
    template<class T>
    PropertyHandle<T> get_property(const std::string& path)
    {
        return PropertyHandle<T>(Path(path));
    }
    /// @}

    /// @{
    /// Searches for and returns a Node in the SceneGraph.
    /// @param path     Path uniquely identifying a Node in the SceneGraph.
    /// @returns        Handle to the requested Node.
    ///                 Is empty if the Node doesn't exist or is of the wrong type.
    /// @throws Path::path_error    If the Path is invalid.
    template<class T = Node>
    NodeHandle<T> get_node(const Path& path)
    {
        return NodeHandle<T>(std::dynamic_pointer_cast<T>(_get_node(path)));
    }
    template<class T = Node>
    NodeHandle<T> get_node(const std::string& path)
    {
        return get_node<T>(Path(path));
    }
    /// @}

    // freezing ---------------------------------------------------------------

    /// Checks if the SceneGraph currently frozen or not.
    bool is_frozen() const { return (m_freezing_thread != 0); }

    /// Checks if the SceneGraph is currently frozen by a given thread.
    /// @param thread_id    Id of the thread in question.
    bool is_frozen_by(std::thread::id thread_id) const { return (m_freezing_thread == hash(thread_id)); }

    // composition ------------------------------------------------------------

    /// The current Composition of the SceneGraph.
    CompositionPtr get_current_composition() const;

    /// Schedule this SceneGraph to switch to a new Composition.
    /// Generates a CompositionChangeEvent and pushes it onto the event queue for the Window.
    void change_composition(CompositionPtr composition);

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
    NodePropertyPtr _get_property(const Path& path);

    /// Private and untyped implementation of `node` assuming sanitized inputs.
    /// @param path     Path uniquely identifying a Node.
    /// @returns        Handle to the requested Node. Is empty if the Node doesn't exist.
    /// @throws Path::path_error    If the Path does not lead to a Node.
    NodePtr _get_node(const Path& path);

    // freezing ---------------------------------------------------------------

    /// Creates and returns a FreezeGuard that keeps the scene frozen while it is alive.
    /// @param thread_id    Id of the freezing thread (should only be used in tests).
    FreezeGuard _freeze_guard(const std::thread::id thread_id = std::this_thread::get_id())
    {
        return FreezeGuard(this, std::move(thread_id));
    }

    /// Freezes the Scene if it is not already frozen.
    /// @param thread_id    Id of the calling thread.
    /// @returns            True iff the Scene was frozen.
    bool _freeze(const std::thread::id thread_id);

    /// Unfreezes the Scene again.
    /// @param thread_id    Id of the calling thread (for safety reasons).
    void _unfreeze(const std::thread::id thread_id);

    // composition ------------------------------------------------------------

    /// Enters a given Composition.
    /// @param composition  New Composition.
    void _set_composition(valid_ptr<CompositionPtr> composition);

    /// Deletes all Nodes and Scenes in the SceneGraph before it is destroyed.
    void _clear();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Window owning this SceneGraph.
    WindowWeakPtr m_window;

    /// Current Composition of the SceneGraph.
    valid_ptr<CompositionPtr> m_current_composition;

    /// Frozen Composition of the SceneGraph.
    risky_ptr<CompositionPtr> m_frozen_composition = nullptr;

    /// Nodes that registered themselves as "dirty".
    /// If there is one or more dirty Nodes registered, the Window containing this graph must be re-rendered.
    std::unordered_set<valid_ptr<Node*>, pointer_hash<valid_ptr<Node*>>> m_dirty_nodes;

    /// All Scenes of this SceneGraph by name.
    SceneMap m_scenes;

    /// All Layers of this SceneGraph.
    std::vector<LayerWeakPtr> m_layers;

    /// Mutex locked while an event is being processed.
    /// This mutex is also aquired by the RenderManager to freeze and unfreeze the graph in between events.
    /// If both mutexes are frozen by the same function, the event mutex is always held longer! (to avoid deadlocks).
    Mutex m_event_mutex;

    /// Mutex guarding the scene.
    /// Needs to be recursive, because deleting a NodeHandle will require the use of the hierarchy mutex, in case
    /// the graph is currently frozen. However, the deletion will also delete all of the node's children who *also* need
    /// the hierachy mutex in case the graph is currently frozen. QED.
    mutable RecursiveMutex m_hierarchy_mutex;

    /// Thread that has frozen the SceneGraph (is 0 if the graph is not frozen).
    std::atomic_size_t m_freezing_thread{0};
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_SceneGraph<Window> {
    friend class notf::Window;

    /// Factory.
    /// @param window   Window owning this SceneGraph.
    static SceneGraphPtr create(valid_ptr<WindowPtr> window) { return SceneGraph::_create(std::move(window)); }

    /// Deletes all Nodes and Scenes in the SceneGraph before it is destroyed.
    static void clear(SceneGraph& graph) { graph._clear(); }
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
        return graph.m_scenes.emplace(std::move(name), SceneWeakPtr{});
    }

    /// Registers a new Scene with the graph.
    /// @param graph    SceneGraph to operate on.
    /// @param scene    Scene to register.
    static void register_scene(SceneGraph& graph, ScenePtr scene);

    /// Direct access to the Graph's event mutex.
    /// @param graph    SceneGraph to operate on.
    static Mutex& event_mutex(SceneGraph& graph) { return graph.m_event_mutex; }

    /// Direct access to the Graph's hierachy mutex.
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& hierarchy_mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
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
    static RecursiveMutex& hierarchy_mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
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
    /// @returns            True iff the Scene was frozen.
    static SceneGraph::FreezeGuard freeze(SceneGraph& graph) { return graph._freeze_guard(std::this_thread::get_id()); }
};

//-----------------------------------------------------------------------------

class access::_SceneGraph_NodeHandle {
    template<class>
    friend struct notf::NodeHandle;

    /// Direct access to the Graph's hierachy mutex.Node
    /// @param graph    SceneGraph to operate on.
    static RecursiveMutex& hierarchy_mutex(SceneGraph& graph) { return graph.m_hierarchy_mutex; }
};

NOTF_CLOSE_NAMESPACE
