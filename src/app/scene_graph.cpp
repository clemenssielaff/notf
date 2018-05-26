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

//====================================================================================================================//

SceneGraph::SceneGraph(Window& window) : m_window(window), m_current_state(create_state({})) {}

SceneGraph::~SceneGraph() = default;

SceneGraph::StatePtr SceneGraph::current_state()
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
    EventManager& event_manger = Application::instance().event_manager();
    event_manger.handle(std::make_unique<StateChangeEvent>(&m_window, std::move(new_state)));
}

void SceneGraph::_register_dirty(valid_ptr<SceneNode*> node)
{
    NOTF_MUTEX_GUARD(m_hierarchy_mutex);

    if (m_dirty_nodes.empty()) {
        m_window.request_redraw();
    }
    m_dirty_nodes.insert(node);
}

void SceneGraph::_remove_dirty(valid_ptr<SceneNode*> node)
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *event);
            }
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::handle_event(*scene, *untyped_event);
            }
        }
    }

    // WindowResizeEvent
    else if (event_type == window_resize_event) {
        WindowResizeEvent* event = static_cast<WindowResizeEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (!layer->is_fullscreen()) {
                continue;
            }
            if (risky_ptr<Scene*> scene = layer->scene()) {
                Scene::Access<SceneGraph>::resize_view(*scene, event->new_size);
            }
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
                    hash(thread_id), hash(m_freezing_thread));

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
        m_window.request_redraw();
    }
}

// ------------------------------------------------------------------------------------------------------------------ //

void access::_SceneGraph<Scene>::register_scene(SceneGraph& graph, ScenePtr scene)
{
    NOTF_ASSERT(graph.m_hierarchy_mutex.is_locked_by_this_thread());
    NOTF_ASSERT(graph.m_scenes.count(scene->name()) == 1); // Scene should have already reserved its name
    graph.m_scenes[scene->name()] = std::move(scene);
}

NOTF_CLOSE_NAMESPACE
