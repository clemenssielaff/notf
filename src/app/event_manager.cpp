#include "event_manager.hpp"

#include <algorithm>

#include "app/glfw.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/window_event.hpp"
#include "app/scene_manager.hpp"
#include "common/log.hpp"
#include "window.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// The current state of all keyboard keys.
KeyStateSet g_key_states;

/// Currently pressed key modifiers.
KeyModifiers g_key_modifiers;

/// The current state of all mouse buttons.
ButtonStateSet g_button_states;

/// Current position of the mouse cursor in screen coordinates.
Vector2f g_cursor_pos;

/// Previous position of the mouse cursor in screen coordinates.
Vector2f g_prev_cursor_pos;

} // namespace

//====================================================================================================================//

void EventManager::WindowHandler::start()
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        if (m_is_running) {
            return;
        }
        m_is_running = true;
        events.clear();
    }
    m_thread = ScopedThread(std::thread(&WindowHandler::_run, this));
}

void EventManager::WindowHandler::enqueue_event(EventPtr&& event)
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        events.emplace_back(std::move(event));
    }
    m_condition.notify_one();
}

void EventManager::WindowHandler::stop()
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        if (!m_is_running) {
            return;
        }
        m_is_running = false;
        events.clear();
    }
    m_condition.notify_one();
    m_thread = {}; // blocks
}

void EventManager::WindowHandler::_run()
{
    // TODO: forward all event objects to the SceneManager that will then propagate to the Scenes
}

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

EventManager::EventManager()
{
    // set the error callback to catch all GLFW errors
    glfwSetErrorCallback(EventManager::_on_error);
}

EventManager::~EventManager() = default;

void EventManager::_propagate_event_to(EventPtr&& event, Window* window)
{
    auto handler_it
        = std::find_if(m_handler.begin(), m_handler.end(), [&](const std::unique_ptr<WindowHandler>& handler) -> bool {
              return handler->window == window;
          });
    if (handler_it == m_handler.end()) {
        log_critical << "Cannot find event handler";
        return;
    }

    WindowHandler* handler = handler_it->get();
    handler->enqueue_event(std::move(event));
}

void EventManager::_on_error(int error_number, const char* message)
{
#ifdef NOTF_DEBUG
    static const std::string glfw_not_initialized = "GLFW_NOT_INITIALIZED";
    static const std::string glfw_no_current_context = "GLFW_NO_CURRENT_CONTEXT";
    static const std::string glfw_invalid_enum = "GLFW_INVALID_ENUM";
    static const std::string glfw_invalid_value = "GLFW_INVALID_VALUE";
    static const std::string glfw_out_of_memory = "GLFW_OUT_OF_MEMORY";
    static const std::string glfw_api_unavailable = "GLFW_API_UNAVAILABLE";
    static const std::string glfw_version_unavailable = "GLFW_VERSION_UNAVAILABLE";
    static const std::string glfw_platform_error = "GLFW_PLATFORM_ERROR";
    static const std::string glfw_format_unavailable = "GLFW_FORMAT_UNAVAILABLE";
    static const std::string glfw_no_window_context = "GLFW_NO_WINDOW_CONTEXT";
    static const std::string unknown_error = "<unknown error>";

    const std::string* error_name;
    switch (error_number) {
    case GLFW_NOT_INITIALIZED:
        error_name = &glfw_not_initialized;
        break;
    case GLFW_NO_CURRENT_CONTEXT:
        error_name = &glfw_no_current_context;
        break;
    case GLFW_INVALID_ENUM:
        error_name = &glfw_invalid_enum;
        break;
    case GLFW_INVALID_VALUE:
        error_name = &glfw_invalid_value;
        break;
    case GLFW_OUT_OF_MEMORY:
        error_name = &glfw_out_of_memory;
        break;
    case GLFW_API_UNAVAILABLE:
        error_name = &glfw_api_unavailable;
        break;
    case GLFW_VERSION_UNAVAILABLE:
        error_name = &glfw_version_unavailable;
        break;
    case GLFW_PLATFORM_ERROR:
        error_name = &glfw_platform_error;
        break;
    case GLFW_FORMAT_UNAVAILABLE:
        error_name = &glfw_format_unavailable;
        break;
    case GLFW_NO_WINDOW_CONTEXT:
        error_name = &glfw_no_window_context;
        break;
    default:
        error_name = &unknown_error;
    }
    log_critical << "GLFW Error " << *error_name << " (" << error_number << "): '" << message << "'";
#else
    log_critical << "GLFW Error (" << error_number << "): '" << message << "'";
#endif
}

void EventManager::_on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    // parse raw arguments
    Button notf_button = static_cast<Button>(button);
    MouseAction notf_action = static_cast<MouseAction>(action);
    NOTF_ASSERT(notf_action == MouseAction::PRESS || notf_action == MouseAction::RELEASE);

    // update the global state
    set_button(g_button_states, notf_button, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // invert the y-coordinate (by default, y grows down)
    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
    window_pos.y() = window->window_size().height - window_pos.y();

    // let the window handle the event
    EventPtr event = std::make_unique<MouseEvent>(
        Vector2f{g_cursor_pos.x() - static_cast<float>(window_pos.x()),
                 g_cursor_pos.y() - static_cast<float>(window_pos.y())}, // event position in window coordinates
        Vector2f::zero(), notf_button, notf_action, g_key_modifiers, g_button_states);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_cursor_move(GLFWwindow* glfw_window, double x, double y)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    { // update the global state
        g_prev_cursor_pos = g_cursor_pos;

        // invert the y-coordinate (by default, y grows down)
        const int window_height = window->window_size().height;
        y = window_height - y;
        Vector2i window_pos;
        glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
        window_pos.y() = window_height - window_pos.y();

        g_cursor_pos.x() = static_cast<float>(window_pos.x()) + static_cast<float>(x);
        g_cursor_pos.y() = static_cast<float>(window_pos.y()) + static_cast<float>(y);
    }

    // let the window handle the event
    EventPtr event = std::make_unique<MouseEvent>(
        Vector2f{static_cast<float>(x), static_cast<float>(y)}, // event position in window coordinates
        g_cursor_pos - g_prev_cursor_pos,                       // delta in window coordinates
        Button::NO_BUTTON,                                      // move events are triggered by no button
        MouseAction::MOVE, g_key_modifiers, g_button_states);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_cursor_entered(GLFWwindow* glfw_window, int entered)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    EventPtr event = std::make_unique<WindowEvent>(entered == GLFW_TRUE ? WindowEvent::Type::CURSOR_ENTERED :
                                                                          WindowEvent::Type::CURSOR_EXITED);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_scroll(GLFWwindow* glfw_window, double x, double y)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    // invert the y-coordinate (by default, y grows down)
    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
    window_pos.y() = window->window_size().height - window_pos.y();

    // let the window handle the event
    EventPtr event = std::make_unique<MouseEvent>(
        Vector2f{g_cursor_pos.x() - static_cast<float>(window_pos.x()),
                 g_cursor_pos.y() - static_cast<float>(window_pos.y())}, // event position in window coordinates
        Vector2f{static_cast<float>(x), static_cast<float>(-y)},         // delta in window coordinates
        Button::NO_BUTTON, MouseAction::SCROLL, g_key_modifiers, g_button_states);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_token_key(GLFWwindow* glfw_window, int key, NOTF_UNUSED int scancode, int action, int modifiers)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    // update the global state
    Key notf_key = from_glfw_key(key);
    set_key(g_key_states, notf_key, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // let the window handle the event
    EventPtr event = std::make_unique<KeyEvent>(notf_key, KeyAction(action), KeyModifiers(modifiers), g_key_states);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_char_input(GLFWwindow* glfw_window, uint codepoint) { _on_shortcut(glfw_window, codepoint, 0); }

void EventManager::_on_shortcut(GLFWwindow* glfw_window, uint codepoint, int modifiers)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    EventPtr event = std::make_unique<CharEvent>(codepoint, KeyModifiers(modifiers), g_key_states);
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_window_move(GLFWwindow* /*glfw_window*/, int /*x*/, int /*y*/) { NOTF_NOOP; }

void EventManager::_on_window_resize(GLFWwindow* glfw_window, int width, int height)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    EventPtr event = std::make_unique<WindowResizeEvent>(window->window_size(),  // old size
                                                         Size2i{width, height}); // new size
    Application::instance().event_manager()._propagate_event_to(std::move(event), window);
}

void EventManager::_on_framebuffer_resize(GLFWwindow* /*glfw_window*/, int /*width*/, int /*height*/) { NOTF_NOOP; }

void EventManager::_on_window_refresh(GLFWwindow* glfw_window)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->request_redraw();
}

void EventManager::_on_window_focus(GLFWwindow* /*glfw_window*/, int /*kind*/) { NOTF_NOOP; }

void EventManager::_on_window_minimize(GLFWwindow* /*glfw_window*/, int /*kind*/) { NOTF_NOOP; }

void EventManager::_on_file_drop(GLFWwindow* /*glfw_window*/, int /*count*/, const char** /*paths*/) { NOTF_NOOP; }

void EventManager::_on_window_close(GLFWwindow* glfw_window)
{
    valid_ptr<Window*> window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->close();
}

void EventManager::_on_monitor_change(GLFWmonitor* /*glfw_monitor*/, int /*kind*/) { NOTF_NOOP; }

void EventManager::_on_joystick_change(int /*joystick*/, int /*kind*/) { NOTF_NOOP; }

// ===================================================================================================================//

void EventManager::Access<Window>::register_window(Window& window)
{
    // create the handler
    EventManager& manager = Application::instance().event_manager();
    for (const auto& handler : manager.m_handler) {
        if (handler->window == &window) {
            log_critical << "Ignoring registration of duplicate Window: " << window.title();
            return;
        }
    }
    manager.m_handler.emplace_back(std::make_unique<WindowHandler>(&window));
    WindowHandler& handler = *manager.m_handler.back().get();

    //
    // register all glfw callbacks
    GLFWwindow* glfw_window = Window::Access<EventManager>(window).glfw_window();

    // input callbacks
    glfwSetMouseButtonCallback(glfw_window, EventManager::_on_mouse_button);
    glfwSetCursorPosCallback(glfw_window, EventManager::_on_cursor_move);
    glfwSetCursorEnterCallback(glfw_window, EventManager::_on_cursor_entered);
    glfwSetScrollCallback(glfw_window, EventManager::_on_scroll);
    glfwSetKeyCallback(glfw_window, EventManager::_on_token_key);
    glfwSetCharCallback(glfw_window, EventManager::_on_char_input);
    glfwSetCharModsCallback(glfw_window, EventManager::_on_shortcut);

    // window callbacks
    glfwSetWindowPosCallback(glfw_window, EventManager::_on_window_move);
    glfwSetWindowSizeCallback(glfw_window, EventManager::_on_window_resize);
    glfwSetFramebufferSizeCallback(glfw_window, EventManager::_on_framebuffer_resize);
    glfwSetWindowRefreshCallback(glfw_window, EventManager::_on_window_refresh);
    glfwSetWindowFocusCallback(glfw_window, EventManager::_on_window_focus);
    glfwSetDropCallback(glfw_window, EventManager::_on_file_drop);
    glfwSetWindowIconifyCallback(glfw_window, EventManager::_on_window_minimize);
    glfwSetWindowCloseCallback(glfw_window, EventManager::_on_window_close);

    // other callbacks
    glfwSetMonitorCallback(EventManager::_on_monitor_change);
    glfwSetJoystickCallback(EventManager::_on_joystick_change);

    handler.start();
}

void EventManager::Access<Window>::remove_window(Window& window)
{
    // disconnect the window callbacks
    GLFWwindow* glfw_window = Window::Access<EventManager>(window).glfw_window();
    glfwSetMouseButtonCallback(glfw_window, nullptr);
    glfwSetCursorPosCallback(glfw_window, nullptr);
    glfwSetCursorEnterCallback(glfw_window, nullptr);
    glfwSetScrollCallback(glfw_window, nullptr);
    glfwSetKeyCallback(glfw_window, nullptr);
    glfwSetCharCallback(glfw_window, nullptr);
    glfwSetCharModsCallback(glfw_window, nullptr);
    glfwSetWindowPosCallback(glfw_window, nullptr);
    glfwSetWindowSizeCallback(glfw_window, nullptr);
    glfwSetFramebufferSizeCallback(glfw_window, nullptr);
    glfwSetWindowRefreshCallback(glfw_window, nullptr);
    glfwSetWindowFocusCallback(glfw_window, nullptr);
    glfwSetDropCallback(glfw_window, nullptr);
    glfwSetWindowIconifyCallback(glfw_window, nullptr);
    glfwSetWindowCloseCallback(glfw_window, nullptr);
    glfwSetMonitorCallback(nullptr);
    glfwSetJoystickCallback(nullptr);

    // remove handler
    EventManager& manager = Application::instance().event_manager();

    auto it = std::find_if(manager.m_handler.begin(), manager.m_handler.end(),
                           [&](const std::unique_ptr<WindowHandler>& handler) -> bool {
                               return handler->window == &window;
                           });
    if (it == manager.m_handler.end()) {
        log_critical << "Ignoring unregistration of unknown Window: " << window.title();
        return;
    }

    WindowHandler* handler = it->get();
    handler->stop();

    *it = std::move(manager.m_handler.back());
    manager.m_handler.pop_back();
}

NOTF_CLOSE_NAMESPACE
