#include "event_manager.hpp"

#include <algorithm>

#include "app/glfw.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/keyboard.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/time.hpp"
#include "app/io/window_event.hpp"
#include "common/exception.hpp"
#include "common/log.hpp"
#include "io/event.hpp"
#include "window.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

// TODO: move all of the global input states to the event manager

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

NOTF_OPEN_NAMESPACE

EventManager::EventManager() = default;

EventManager::~EventManager() = default;

void EventManager::add_window(Window& window)
{
    for (const auto& handler : m_handler) {
        if (handler.window == &window) {
            log_critical << "Ignoring registration of duplicate Window: " << window.title();
            return;
        }
    }
    m_handler.emplace_back(WindowHandler{&window, {}});
}

void EventManager::remove_window(Window& window)
{
    auto it = std::find_if(m_handler.begin(), m_handler.end(),
                           [&](const WindowHandler& handler) -> bool { return handler.window == &window; });
    if (it == m_handler.end()) {
        log_critical << "Ignoring unregistration of unknown Window: " << window.title();
        return;
    }
    *it = std::move(m_handler.back());
    m_handler.pop_back();
}

void EventManager::on_error(int error_number, const char* message)
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

void EventManager::on_token_key(GLFWwindow* glfw_window, int key, NOTF_UNUSED int scancode, int action, int modifiers)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    // update the global state
//    Key notf_key = from_glfw_key(key);
//    set_key(g_key_states, notf_key, action);
//    g_key_modifiers = KeyModifiers(modifiers);

//    // let the window handle the event
//    Window::Access<Application>(*window).propagate(
//        KeyEvent(*window, notf_key, KeyAction(action), KeyModifiers(modifiers), g_key_states));
}

void EventManager::on_char_input(GLFWwindow* glfw_window, uint codepoint, int modifiers)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    // let the window handle the event
//    Window::Access<Application>(*window).propagate(
//        CharEvent(*window, codepoint, KeyModifiers(modifiers), g_key_states));
}

void EventManager::on_cursor_entered(GLFWwindow* glfw_window, int entered)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    if (entered) {
//        Window::Access<Application>(*window).propagate(WindowEvent(*window, WindowEvent::Type::CURSOR_ENTERED));
//    }
//    else {
//        Window::Access<Application>(*window).propagate(WindowEvent(*window, WindowEvent::Type::CURSOR_EXITED));
//    }
}

void EventManager::on_cursor_move(GLFWwindow* glfw_window, double x, double y)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    { // update the global state
//        g_prev_cursor_pos = g_cursor_pos;

//        // invert the y-coordinate (by default, y grows down)
//        const int window_height = window->window_size().height;
//        y = window_height - y;
//        Vector2i window_pos;
//        glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
//        window_pos.y() = window_height - window_pos.y();

//        g_cursor_pos.x() = static_cast<float>(window_pos.x()) + static_cast<float>(x);
//        g_cursor_pos.y() = static_cast<float>(window_pos.y()) + static_cast<float>(y);
//    }

//    // let the window handle the event
//    MouseEvent mouse_event(*window,
//                           {static_cast<float>(x), static_cast<float>(y)}, // event position in window coordinates
//                           g_cursor_pos - g_prev_cursor_pos,               // delta in window coordinates
//                           Button::NO_BUTTON,                              // move events are triggered by no button
//                           MouseAction::MOVE, g_key_modifiers, g_button_states);
//    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void EventManager::on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    // parse raw arguments
//    Button notf_button = static_cast<Button>(button);
//    MouseAction notf_action = static_cast<MouseAction>(action);
//    assert(notf_action == MouseAction::PRESS || notf_action == MouseAction::RELEASE);

//    // update the global state
//    set_button(g_button_states, notf_button, action);
//    g_key_modifiers = KeyModifiers(modifiers);

//    // invert the y-coordinate (by default, y grows down)
//    Vector2i window_pos;
//    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
//    window_pos.y() = window->window_size().height - window_pos.y();

//    // let the window handle the event
//    MouseEvent mouse_event(*window,
//                           {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
//                            g_cursor_pos.y() - static_cast<float>(window_pos.y())},
//                           Vector2f::zero(), notf_button, notf_action, g_key_modifiers, g_button_states);
//    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void EventManager::on_scroll(GLFWwindow* glfw_window, double x, double y)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);

//    // invert the y-coordinate (by default, y grows down)
//    Vector2i window_pos;
//    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
//    window_pos.y() = window->window_size().height - window_pos.y();

//    // let the window handle the event
//    MouseEvent mouse_event(*window,
//                           {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
//                            g_cursor_pos.y() - static_cast<float>(window_pos.y())},
//                           {static_cast<float>(x), static_cast<float>(-y)}, Button::NO_BUTTON, MouseAction::SCROLL,
//                           g_key_modifiers, g_button_states);
//    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void EventManager::on_window_close(GLFWwindow* glfw_window)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);
    window->close();
}

void EventManager::on_window_resize(GLFWwindow* glfw_window, int width, int height)
{
//    assert(glfw_window);
//    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//    assert(window);
//    Window::Access<Application>(*window).resize({width, height});
}

NOTF_CLOSE_NAMESPACE
