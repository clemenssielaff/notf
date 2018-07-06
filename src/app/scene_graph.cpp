#include "app/scene_graph.hpp"

#include "app/event_manager.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/state_event.hpp"
#include "app/io/window_event.hpp"
#include "app/layer.hpp"
#include "app/window.hpp"
#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

SceneGraph::SceneGraph(valid_ptr<WindowPtr> window)
    : m_window(std::move(window).raw()), m_current_state(create_state({}))
{}

SceneGraph::~SceneGraph() = default;

SceneGraph::StatePtr SceneGraph::current_state() const
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (is_frozen_by(std::this_thread::get_id())) {
        NOTF_ASSERT(m_frozen_state);
        return m_frozen_state;
    }
    return m_current_state;
}

void SceneGraph::enter_state(StatePtr new_state)
{
    risky_ptr<WindowPtr> window = get_window();
    if (NOTF_UNLIKELY(!window)) {
        return;
    }
    EventManager& event_manger = Application::instance().event_manager();
    event_manger.handle(std::make_unique<StateChangeEvent>(raw_pointer(window), std::move(new_state)));
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
    static const size_t state_change_event = StateChangeEvent::static_type();

    NOTF_MUTEX_GUARD(m_event_mutex);

    const size_t event_type = untyped_event->type();

    // MouseEvent
    if (event_type == mouse_event) {
        MouseEvent* event = static_cast<MouseEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->scene(), *event);
            if (event->was_handled()) { // TODO: maybe add virtual "is_handled" function? Or "is_handleable_ subtype
                break;                  // this way we wouldn't have to differentiate between Mouse, Key, CharEvents
            }                           // they are all handled the same anyway
        }
    }

    // KeyEvent
    else if (event_type == key_event) {
        KeyEvent* event = static_cast<KeyEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->scene(), *event);
            if (event->was_handled()) {
                break;
            }
        }
    }

    // CharEvent
    else if (event_type == char_event) {
        CharEvent* event = static_cast<CharEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->scene(), *event);
            if (event->was_handled()) {
                break;
            }
        }
    }

    // WindowEvent
    else if (event_type == window_event) {
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_active()) {
                continue;
            }
            Scene::Access<SceneGraph>::handle_event(layer->scene(), *untyped_event);
        }
    }

    // WindowResizeEvent
    else if (event_type == window_resize_event) {
        WindowResizeEvent* event = static_cast<WindowResizeEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_fullscreen()) {
                continue;
            }
            Scene::Access<SceneGraph>::resize_view(layer->scene(), event->new_size);
        }
    }

    // StateChangeEvent
    else if (event_type == state_change_event) {
        StateChangeEvent* event = static_cast<StateChangeEvent*>(untyped_event.get());
        _enter_state(event->new_state);
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
        m_frozen_state = m_current_state;

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

void SceneGraph::_enter_state(valid_ptr<StatePtr> state)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);
    if (state != m_current_state) {
        m_current_state = std::move(state);
        risky_ptr<WindowPtr> window = get_window();
        if (NOTF_LIKELY(window)) {
            raw_pointer(window)->request_redraw();
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
