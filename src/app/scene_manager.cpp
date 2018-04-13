#include "app/scene_manager.hpp"

#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/window_event.hpp"
#include "app/layer.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"

NOTF_OPEN_NAMESPACE

SceneManager::SceneManager(Window& window) : m_window(window), m_current_state(create_state({})) {}

SceneManager::~SceneManager() = default;

void SceneManager::propagate_event(EventPtr&& untyped_event)
{
    static const size_t char_event = CharEvent::static_type();
    static const size_t key_event = KeyEvent::static_type();
    static const size_t mouse_event = MouseEvent::static_type();
    static const size_t window_event = WindowEvent::static_type();
    static const size_t window_resize_event = WindowResizeEvent::static_type();

    const size_t event_type = untyped_event->type();

    // MouseEvent
    if (event_type == mouse_event) {
        MouseEvent* event = static_cast<MouseEvent*>(untyped_event.get());
        for (auto& layer : m_current_state->layers()) {
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
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
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*event);
            }
            if (event->was_handled()) {
                break;
            }
        }
    }

    // WindowEvent
    else if (event_type == window_event) {
        for (auto& layer : m_current_state->layers()) {
            if (risky_ptr<Scene*> scene = layer->scene()) {
                scene->handle_event(*untyped_event);
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
                scene->resize_view(event->new_size);
            }
        }
    }
}

void SceneManager::enter_state(StatePtr state)
{
    m_current_state = std::move(state);
    m_window.request_redraw();
}

NOTF_CLOSE_NAMESPACE
