#include "app/scene_graph.hpp"

#include "app/event_manager.hpp"
#include "app/io/char_event.hpp"
#include "app/io/composition_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/window_event.hpp"
#include "app/scene.hpp"
#include "app/visualizer/visualizer.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

SceneGraph::no_window_error::~no_window_error() = default;

// ================================================================================================================== //

SceneGraph::Layer::Layer(valid_ptr<ScenePtr> scene, valid_ptr<VisualizerPtr> visualizer)
    : m_scene(std::move(scene)), m_visualizer(std::move(visualizer.raw()))
{}

SceneGraph::LayerPtr SceneGraph::Layer::create(valid_ptr<ScenePtr> scene, valid_ptr<VisualizerPtr> visualizer)
{
    LayerPtr result = NOTF_MAKE_SHARED_FROM_PRIVATE(Layer, std::move(scene), std::move(visualizer));
    SceneGraph& scene_graph = result->get_scene().get_graph();
    {
        NOTF_MUTEX_GUARD(scene_graph.m_hierarchy_mutex);
        scene_graph.m_layers.emplace_back(result);
    }
    return result;
}

void SceneGraph::Layer::draw()
{
    if (!is_visible()) {
        return;
    }

    Scene& scene = get_scene(); // throws if the Window has been closed
    SceneGraph& scene_graph = scene.get_graph();
    NOTF_ASSERT(scene_graph.is_frozen_by(std::this_thread::get_id()));

    // define render area
    WindowPtr window = scene_graph.get_window();
    GraphicsContext& context = window->get_graphics_context();
    if (m_is_fullscreen) {
        context.set_render_area(context.get_window_size());
    }
    else {
        if (m_area.is_zero()) {
            return;
        }
        if (!m_area.is_valid()) {
            log_warning << "Cannot draw a Layer with an invalid area";
            return;
        }
        context.set_render_area(m_area);
    }

    // the render thread should never modify the hierarchy
    NOTF_ASSERT(scene_graph.is_frozen_by(std::this_thread::get_id()));
    m_visualizer->visualize(raw_pointer(m_scene));
}

// ================================================================================================================== //

SceneGraph::SceneGraph(valid_ptr<WindowPtr> window)
    : m_window(std::move(window).raw()), m_current_composition(Composition::create({}))
{}

SceneGraph::~SceneGraph() = default;

SceneGraph::CompositionPtr SceneGraph::get_current_composition() const
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (is_frozen_by(std::this_thread::get_id())) {
        NOTF_ASSERT(m_frozen_composition);
        return m_frozen_composition;
    }
    return m_current_composition;
}

void SceneGraph::change_composition(CompositionPtr composition)
{
    risky_ptr<WindowPtr> window = get_window();
    if (NOTF_UNLIKELY(!window)) {
        return;
    }
    EventManager& event_manger = Application::instance().get_event_manager();
    event_manger.handle(std::make_unique<CompositionChangeEvent>(raw_pointer(window), std::move(composition)));
}

void SceneGraph::_register_dirty(valid_ptr<Node*> node)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (m_dirty_nodes.empty()) {
        risky_ptr<WindowPtr> window = get_window();
        if (NOTF_LIKELY(window)) {
            raw_pointer(window)->request_redraw();
        }
    }
    m_dirty_nodes.insert(node);
}

void SceneGraph::_remove_dirty(valid_ptr<Node*> node)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    m_dirty_nodes.erase(node);
}

void SceneGraph::_propagate_event(EventPtr&& untyped_event)
{
    static const size_t char_event = CharEvent::static_type();
    static const size_t key_event = KeyEvent::static_type();
    static const size_t mouse_event = MouseEvent::static_type();
    static const size_t window_event = WindowEvent::static_type();
    static const size_t window_resize_event = WindowResizeEvent::static_type();
    static const size_t composition_change_event = CompositionChangeEvent::static_type();

    NOTF_MUTEX_GUARD(m_event_mutex);

    const size_t event_type = untyped_event->type();

    // MouseEvent
    if (event_type == mouse_event) {
        MouseEvent* event = static_cast<MouseEvent*>(untyped_event.get());
        for (auto& layer : m_current_composition->get_layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->get_scene(), *event);
            if (event->was_handled()) { // TODO: maybe add virtual "is_handled" function? Or "is_handleable_ subtype
                break;                  // this way we wouldn't have to differentiate between Mouse, Key, CharEvents
            }                           // they are all handled the same anyway
        }
    }

    // KeyEvent
    else if (event_type == key_event) {
        KeyEvent* event = static_cast<KeyEvent*>(untyped_event.get());
        for (auto& layer : m_current_composition->get_layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->get_scene(), *event);
            if (event->was_handled()) {
                break;
            }
        }
    }

    // CharEvent
    else if (event_type == char_event) {
        CharEvent* event = static_cast<CharEvent*>(untyped_event.get());
        for (auto& layer : m_current_composition->get_layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->get_scene(), *event);
            if (event->was_handled()) {
                break;
            }
        }
    }

    // WindowEvent
    else if (event_type == window_event) {
        for (auto& layer : m_current_composition->get_layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->get_scene(), *untyped_event);
        }
    }

    // WindowResizeEvent
    else if (event_type == window_resize_event) {
        WindowResizeEvent* event = static_cast<WindowResizeEvent*>(untyped_event.get());
        for (auto& layer : m_current_composition->get_layers()) {
            if (!layer->is_fullscreen()) {
                continue;
            }
            Scene::Access<SceneGraph>::resize_view(layer->get_scene(), event->new_size);
        }
    }

    // CompositionChangeEvent
    else if (event_type == composition_change_event) {
        CompositionChangeEvent* event = static_cast<CompositionChangeEvent*>(untyped_event.get());
        _set_composition(event->new_composition);
    }

    // fallback
    else {
        log_warning << "Unhandled event of type: " << event_type;
    }
}

NodePropertyPtr SceneGraph::_get_property(const Path& path)
{
    if (path.is_empty()) {
        NOTF_THROW(Path::path_error, "Cannot query a Property from a SceneGraph with an empty path");
    }
    if (!path.is_property()) {
        NOTF_THROW(Path::path_error, "Path \"{}\" does not identify a Property", path.to_string())
    }
    const std::string& scene_name = path[0];
    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);

        auto scene_it = m_scenes.find(scene_name);
        if (scene_it != m_scenes.end()) {
            if (ScenePtr scene = scene_it->second.lock()) {
                return Scene::Access<SceneGraph>::get_property(*scene, path);
            }
        }
    }
    NOTF_THROW(Path::path_error, "Path \"{}\" refers to unknown Scene \"{}\" in SceneGraph", path.to_string(),
               scene_name);
}

NodePtr SceneGraph::_get_node(const Path& path)
{
    if (path.is_empty()) {
        NOTF_THROW(Path::path_error, "Cannot query a Node  from a SceneGraph with an empty path");
    }
    if (!path.is_node()) {
        NOTF_THROW(Path::path_error, "Path \"{}\" does not identify a Node", path.to_string())
    }
    const std::string& scene_name = path[0];
    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);

        auto scene_it = m_scenes.find(scene_name);
        if (scene_it != m_scenes.end()) {
            if (ScenePtr scene = scene_it->second.lock()) {
                return Scene::Access<SceneGraph>::get_node(*scene, path);
            }
        }
    }
    NOTF_THROW(Path::path_error, "Path \"{}\" refers to unknown Scene \"{}\" in SceneGraph", path.to_string(),
               scene_name);
}

bool SceneGraph::_freeze(const std::thread::id thread_id)
{
    NOTF_MUTEX_GUARD(m_event_mutex);

    const size_t thread_hash = hash(thread_id);

    if (m_freezing_thread == thread_hash) {
        log_warning << "Ignoring repeated freezing of SceneGraph from the same thread";
        return false;
    }
    if (m_freezing_thread != 0) {
        log_critical << "Ignoring duplicate freezing of SceneGraph from another thread";
        return false;
    }

    m_freezing_thread = thread_hash;

    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);
        m_frozen_composition = m_current_composition;

        // remove all dirty nodes, any further changes from this point onward will trigger a new redraw
        m_dirty_nodes.clear();
    }

    return true;
}

void SceneGraph::_unfreeze(NOTF_UNUSED const std::thread::id thread_id)
{
    NOTF_MUTEX_GUARD(m_event_mutex);

    if (m_freezing_thread == 0) {
        return; // already unfrozen
    }
    NOTF_ASSERT_MSG(m_freezing_thread == hash(thread_id),
                    "Thread #{} must not unfreeze the SceneGraph, because it was frozen by a different thread (#{}).",
                    hash(thread_id), hash(m_freezing_thread.load()));

    {
        NOTF_MUTEX_GUARD(m_hierarchy_mutex);

        // unfreeze - otherwise all modifications would just create new deltas
        m_freezing_thread = 0;

        // clear old deltas in all scenes
        for (auto it = m_scenes.begin(); it != m_scenes.end();) {
            if (ScenePtr scene = it->second.lock()) {
                Scene::Access<SceneGraph>::clear_delta(*scene);
                ++it;
            }

            // ...and sort out Scenes that were deleted
            else {
                it = m_scenes.erase(it);
            }
        }
    }
}

void SceneGraph::_set_composition(valid_ptr<CompositionPtr> composition)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);
    if (composition != m_current_composition) {
        m_current_composition = std::move(composition);
        risky_ptr<WindowPtr> window = get_window();
        if (NOTF_LIKELY(window)) {
            raw_pointer(window)->request_redraw();
        }
    }
}

void SceneGraph::_clear()
{
    NOTF_MUTEX_GUARDS(m_event_mutex, 0);
    NOTF_MUTEX_GUARDS(m_hierarchy_mutex, 1);

    // clear all Scenes and Visualizers from the Layers of this SceneGraph
    for (LayerWeakPtr& weak_layer : m_layers) {
        if (LayerPtr layer = weak_layer.lock()) {
            layer->m_scene.reset();      //
            layer->m_visualizer.reset(); // TODO: does this have to be any more thread safe?
        }
    }

    // delete all remaining Scenes, copies of ones that were contained in Layers but also unassociated ones
    for (auto it = m_scenes.begin(); it != m_scenes.end();) {
        if (ScenePtr scene = it->second.lock()) {
            scene->clear();
            ++it;
        }
        else {
            it = m_scenes.erase(it);
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------ //

void access::_SceneGraph<Scene>::register_scene(SceneGraph& graph, ScenePtr scene)
{
    NOTF_ASSERT(graph.m_hierarchy_mutex.is_locked_by_this_thread());
    NOTF_ASSERT(graph.m_scenes.count(scene->get_name()) == 1); // Scene should have already reserved its name
    graph.m_scenes[scene->get_name()] = std::move(scene);
}

NOTF_CLOSE_NAMESPACE
