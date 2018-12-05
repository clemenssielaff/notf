#include "notf/app/glfw_callbacks.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/event_handler.hpp"
#include "notf/app/glfw.hpp"
#include "notf/app/timer_pool.hpp"
#include "notf/app/window.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

WindowHandle to_window_handle(GLFWwindow* glfw_window) {
    Window* raw_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    NOTF_ASSERT(raw_window);
    WindowPtr window = std::dynamic_pointer_cast<Window>(raw_window->shared_from_this()); // {1}
    NOTF_ASSERT(window);
    return WindowHandle(std::move(window));
}

/*
{1}
When running with ThreadSanitizer, this line causes a "data race" false positive, but only if you open the Window from
the UI thread and close it right away, without creating any other event.
Just FYI, if you come across this warning and don't know what to do about it.
*/

} // namespace

NOTF_OPEN_NAMESPACE

// glfw callbacks =================================================================================================== //

void GlfwCallbacks::_on_error(int error, const char* message) {
    static const char* const glfw_not_initialized = "GLFW_NOT_INITIALIZED";
    static const char* const glfw_no_current_context = "GLFW_NO_CURRENT_CONTEXT";
    static const char* const glfw_invalid_enum = "GLFW_INVALID_ENUM";
    static const char* const glfw_invalid_value = "GLFW_INVALID_VALUE";
    static const char* const glfw_out_of_memory = "GLFW_OUT_OF_MEMORY";
    static const char* const glfw_api_unavailable = "GLFW_API_UNAVAILABLE";
    static const char* const glfw_version_unavailable = "GLFW_VERSION_UNAVAILABLE";
    static const char* const glfw_platform_error = "GLFW_PLATFORM_ERROR";
    static const char* const glfw_format_unavailable = "GLFW_FORMAT_UNAVAILABLE";
    static const char* const glfw_no_window_context = "GLFW_NO_WINDOW_CONTEXT";
    static const char* const unknown_error = "unknown error";

    const char* error_name;
    switch (error) {
    case GLFW_NOT_INITIALIZED: error_name = glfw_not_initialized; break;
    case GLFW_NO_CURRENT_CONTEXT: error_name = glfw_no_current_context; break;
    case GLFW_INVALID_ENUM: error_name = glfw_invalid_enum; break;
    case GLFW_INVALID_VALUE: error_name = glfw_invalid_value; break;
    case GLFW_OUT_OF_MEMORY: error_name = glfw_out_of_memory; break;
    case GLFW_API_UNAVAILABLE: error_name = glfw_api_unavailable; break;
    case GLFW_VERSION_UNAVAILABLE: error_name = glfw_version_unavailable; break;
    case GLFW_PLATFORM_ERROR: error_name = glfw_platform_error; break;
    case GLFW_FORMAT_UNAVAILABLE: error_name = glfw_format_unavailable; break;
    case GLFW_NO_WINDOW_CONTEXT: error_name = glfw_no_window_context; break;
    default: error_name = unknown_error;
    }
    NOTF_LOG_CRIT("GLFW Error \"{}\"({}): {}", error_name, error, message);
}

void GlfwCallbacks::_on_mouse_button(GLFWwindow* glfw_window, const int button, const int action, const int modifiers) {
    //        if (action == GLFW_PRESS) {
    //            schedule<MouseButtonPressEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
    //        } else {
    //            NOTF_ASSERT(action == GLFW_RELEASE);
    //            schedule<MouseButtonReleaseEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
    //        }
}

void GlfwCallbacks::_on_cursor_move(GLFWwindow* glfw_window, const double x, const double y) {
    //        schedule<MouseMoveEvent>(glfw_window, V2d{x, y});
}

void GlfwCallbacks::_on_cursor_entered(GLFWwindow* glfw_window, const int entered) {
    //        if (entered == GLFW_TRUE) {
    //            schedule<WindowCursorEnteredEvent>(glfw_window);
    //        } else {
    //            NOTF_ASSERT(entered == GLFW_FALSE);
    //            schedule<WindowCursorExitedEvent>(glfw_window);
    //        }
}

void GlfwCallbacks::_on_scroll(GLFWwindow* glfw_window, const double x, const double y) {
    //        schedule<MouseScrollEvent>(glfw_window, V2d{x, y});
}

void GlfwCallbacks::_on_token_key(GLFWwindow* glfw_window, const int key, const int scancode, const int action,
                                  const int modifiers) {
    //        if (action == GLFW_PRESS) {
    //            schedule<KeyPressEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
    //        } else if (action == GLFW_RELEASE) {
    //            schedule<KeyReleaseEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
    //        } else {
    //            NOTF_ASSERT(action == GLFW_REPEAT);
    //            schedule<KeyRepeatEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
    //        }

    if (action == GLFW_PRESS) {
        using namespace std::chrono_literals;

        if (key == GLFW_KEY_ENTER) {
            TheEventHandler()->schedule([] { Window::create(); });
        } else {
            TheEventHandler()->schedule([] {
                NOTF_USING_LITERALS;
                size_t counter = 0;
                auto timer = IntervalTimer(0.2s,
                                           [counter]() mutable {
                                               for (size_t i = 1, end = counter++; i <= end; ++i) {
                                                   std::cout << " ";
                                               }
                                               std::cout << counter << std::endl;
                                           },
                                           10);
                timer->start(/*detached=*/true);
            });
        }
    }
}

void GlfwCallbacks::_on_char_input(GLFWwindow* glfw_window, const uint codepoint) {
    //        schedule<CharInputEvent>(glfw_window, codepoint);
}

void GlfwCallbacks::_on_shortcut(GLFWwindow* glfw_window, const uint codepoint, const int modifiers) {
    //        schedule<ShortcutEvent>(glfw_window, codepoint, KeyModifiers(modifiers));
}

void GlfwCallbacks::_on_window_move(GLFWwindow* glfw_window, const int x, const int y) {
    //        schedule<WindowMoveEvent>(glfw_window, V2i{x, y});
}

void GlfwCallbacks::_on_window_resize(GLFWwindow* glfw_window, const int width, const int height) {
    //        schedule<WindowResizeEvent>(glfw_window, Size2i{width, height});
}

void GlfwCallbacks::_on_framebuffer_resize(GLFWwindow* glfw_window, const int width, const int height) {
    //        schedule<WindowResolutionChangeEvent>(glfw_window, Size2i{width, height});
}

void GlfwCallbacks::_on_window_refresh(GLFWwindow* glfw_window) {
    //        schedule<WindowRefreshEvent>(glfw_window);
}

void GlfwCallbacks::_on_window_focus(GLFWwindow* glfw_window, const int kind) {
    //        if (kind == GLFW_TRUE) {
    //            schedule<WindowFocusGainEvent>(glfw_window);
    //        } else {
    //            NOTF_ASSERT(kind == GLFW_FALSE);
    //            schedule<WindowFocusLostEvent>(glfw_window);
    //        }
}

void GlfwCallbacks::_on_window_minimize(GLFWwindow* glfw_window, const int kind) {
    //        if (kind == GLFW_TRUE) {
    //            schedule<WindowMinimizeEvent>(glfw_window);
    //        } else {
    //            NOTF_ASSERT(kind == GLFW_FALSE);
    //            schedule<WindowRestoredEvent>(glfw_window);
    //        }
}

void GlfwCallbacks::_on_file_drop(GLFWwindow* glfw_window, const int count, const char** paths) {
    //        schedule<WindowFileDropEvent>(glfw_window, static_cast<uint>(count), paths);
}

void GlfwCallbacks::_on_window_close(GLFWwindow* glfw_window) {
    // TODO: ask the Window if it should really be closed; ignore and unset the flag if not
    TheEventHandler()->schedule(
        [window = to_window_handle(glfw_window)]() mutable { window->call<Window::to_close>(); });
}

void GlfwCallbacks::_on_monitor_change(GLFWmonitor* glfw_monitor, const int kind) {}

void GlfwCallbacks::_on_joystick_change(const int joystick, const int kind) {}

NOTF_CLOSE_NAMESPACE
