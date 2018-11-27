#include "notf/app/application.hpp"

#include "notf/meta/log.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/vector.hpp"
#include "notf/common/version.hpp"

#include "notf/app/glfw.hpp"
#include "notf/app/window.hpp"

#include "notf/app/event/event.hpp"
#include "notf/app/event/key_events.hpp"
#include "notf/app/event/mouse_events.hpp"
#include "notf/app/event/scheduler.hpp"
#include "notf/app/event/window_events.hpp"

// glfw event handler =============================================================================================== //

namespace { // anonymous
NOTF_USING_NAMESPACE;

WindowHandle to_window_handle(GLFWwindow* glfw_window) {
    Window* raw_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    NOTF_ASSERT(raw_window);
    WindowPtr window = std::dynamic_pointer_cast<Window>(raw_window->shared_from_this());
    NOTF_ASSERT(window);
    return WindowHandle(std::move(window));
}

template<class T, class... Args>
void schedule(GLFWwindow* glfw_window, Args... args) {

    TheApplication::get_scheduler().schedule(
        make_unique_aggregate<T>(to_window_handle(glfw_window), std::forward<Args>(args)...));
}

struct GlfwEventHandler {

    /// Called by GLFW in case of an error.
    /// @param error    Error ID.
    /// @param message  Error message;
    static void on_error(int error, const char* message) {
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
        case GLFW_NOT_INITIALIZED:
            error_name = glfw_not_initialized;
            break;
        case GLFW_NO_CURRENT_CONTEXT:
            error_name = glfw_no_current_context;
            break;
        case GLFW_INVALID_ENUM:
            error_name = glfw_invalid_enum;
            break;
        case GLFW_INVALID_VALUE:
            error_name = glfw_invalid_value;
            break;
        case GLFW_OUT_OF_MEMORY:
            error_name = glfw_out_of_memory;
            break;
        case GLFW_API_UNAVAILABLE:
            error_name = glfw_api_unavailable;
            break;
        case GLFW_VERSION_UNAVAILABLE:
            error_name = glfw_version_unavailable;
            break;
        case GLFW_PLATFORM_ERROR:
            error_name = glfw_platform_error;
            break;
        case GLFW_FORMAT_UNAVAILABLE:
            error_name = glfw_format_unavailable;
            break;
        case GLFW_NO_WINDOW_CONTEXT:
            error_name = glfw_no_window_context;
            break;
        default:
            error_name = unknown_error;
        }
        NOTF_LOG_CRIT("GLFW Error \"{}\"({}): {}", *error_name, error, message);
    }

    /// Called when the user presses or releases a mouse button Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param button       The mouse button triggering this callback.
    /// @param action       Mouse button action, is either GLFW_PRESS or GLFW_RELEASE.
    /// @param modifiers    Modifier key bitmask.
    static void on_mouse_button(GLFWwindow* glfw_window, const int button, const int action, const int modifiers) {
        if (action == GLFW_PRESS) {
            schedule<MouseButtonPressEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
        } else {
            NOTF_ASSERT(action == GLFW_RELEASE);
            schedule<MouseButtonReleaseEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
        }
    }

    /// Called when the user moves the mouse inside a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            X coordinate of the cursor in Window coordinates.
    /// @param y            Y coordinate of the cursor in Window coordinates.
    static void on_cursor_move(GLFWwindow* glfw_window, const double x, const double y) {
        schedule<MouseMoveEvent>(glfw_window, V2d{x, y});
    }

    /// Called when the cursor enters or exists the client area of a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param entered      Is GLFW_TRUE if the cursor entered the window's client area, or GLFW_FALSE if it left it.
    static void on_cursor_entered(GLFWwindow* glfw_window, int entered) {
        if (entered == GLFW_TRUE) {
            schedule<WindowCursorEnteredEvent>(glfw_window);
        } else {
            NOTF_ASSERT(entered == GLFW_FALSE);
            schedule<WindowCursorExitedEvent>(glfw_window);
        }
    }

    /// Called when the user scrolls inside the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            Horizontal scroll delta in screen coordinates.
    /// @param y            Vertical scroll delta in screen coordinates.
    static void on_scroll(GLFWwindow* glfw_window, const double x, const double y) {
        schedule<MouseScrollEvent>(glfw_window, V2d{x, y});
    }

    /// Called by GLFW when a key is pressed, repeated or released.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param key          Modified key.
    /// @param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// @param action       The action that triggered this callback.
    /// @param modifiers    Modifier key bitmask.
    static void
    on_token_key(GLFWwindow* glfw_window, const int key, const int scancode, const int action, const int modifiers) {
        if (action == GLFW_PRESS) {
            schedule<KeyPressEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
        } else if (action == GLFW_RELEASE) {
            schedule<KeyReleaseEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
        } else {
            NOTF_ASSERT(action == GLFW_REPEAT);
            schedule<KeyRepeatEvent>(glfw_window, to_key(key), scancode, KeyModifiers(modifiers));
        }
    }

    /// Called by GLFW when an unicode code point was generated.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void on_char_input(GLFWwindow* glfw_window, const uint codepoint) {
        schedule<CharInputEvent>(glfw_window, codepoint);
    }

    /// Called by GLFW when the user presses a key combination with at least one modifier key.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void on_shortcut(GLFWwindow* glfw_window, const uint codepoint, const int modifiers) {
        schedule<ShortcutEvent>(glfw_window, codepoint, KeyModifiers(modifiers));
    }

    /// Called when the Window was moved.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            The new x-coordinate of the top-left corner of the Window, in screen coordinates.
    /// @param y            The new y-coordinate of the top-left corner of the Window, in screen coordinates.
    static void on_window_move(GLFWwindow* glfw_window, const int x, const int y) {
        schedule<WindowMoveEvent>(glfw_window, V2i{x, y});
    }

    /// Called when the Window is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the Window.
    /// @param height       New height of the Window.
    static void on_window_resize(GLFWwindow* glfw_window, const int width, const int height) {
        schedule<WindowResizeEvent>(glfw_window, Size2i{width, height});
    }

    /// Called when the Window's framebuffer is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the framebuffer.
    /// @param height       New height of the framebuffer.
    static void on_framebuffer_resize(GLFWwindow* glfw_window, const int width, const int height) {
        schedule<WindowResolutionChangeEvent>(glfw_window, Size2i{width, height});
    }

    /// Called when the Window is refreshed by the OS.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    static void on_window_refresh(GLFWwindow* glfw_window) { schedule<WindowRefreshEvent>(glfw_window); }

    /// Called when the Window has gained/lost OS focus.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE if the window gained focus, or GLFW_FALSE if it lost it.
    static void on_window_focus(GLFWwindow* glfw_window, const int kind) {
        if (kind == GLFW_TRUE) {
            schedule<WindowFocusGainEvent>(glfw_window);
        } else {
            NOTF_ASSERT(kind == GLFW_FALSE);
            schedule<WindowFocusLostEvent>(glfw_window);
        }
    }

    /// Called when the Window is minimzed.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE when the Window was minimized, or GLFW_FALSE if it was restored.
    static void on_window_minimize(GLFWwindow* glfw_window, const int kind) {
        if (kind == GLFW_TRUE) {
            schedule<WindowMinimizeEvent>(glfw_window);
        } else {
            NOTF_ASSERT(kind == GLFW_FALSE);
            schedule<WindowRestoredEvent>(glfw_window);
        }
    }

    /// Called by GLFW when the user drops one or more files into the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param count        Number of dropped files.
    /// @param paths        The UTF-8 encoded file and/or directory path names.
    static void on_file_drop(GLFWwindow* glfw_window, const int count, const char** paths) {
        schedule<WindowFileDropEvent>(glfw_window, static_cast<uint>(count), paths);
    }

    /// Called by GLFW, if the user requested a window to be closed.
    /// @param glfw_window  GLFW Window to close.
    static void on_window_close(GLFWwindow* glfw_window) { schedule<WindowCloseEvent>(glfw_window); }

    /// Called by GLFW, if the user connects or disconnects a monitor.
    /// @param glfw_monitor The monitor that was connected or disconnected.
    /// @param kind         Either GLFW_CONNECTED or GLFW_DISCONNECTED.
    static void on_monitor_change(GLFWmonitor* /*glfw_monitor*/, const int /*kind*/) {}

    /// Called when a joystick was connected or disconnected.
    /// @param joystick The joystick that was connected or disconnected.
    /// @param kind     One of GLFW_CONNECTED or GLFW_DISCONNECTED.
    static void on_joystick_change(const int /*joystick*/, const int /*kind*/) {}
};

} // namespace

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

TheApplication::Application::Application(Arguments args)
    : m_arguments(std::move(args))
    , m_shared_context(nullptr, detail::window_deleter)
    , m_windows(nullptr)
    , m_scheduler(nullptr) {

    // initialize GLFW
    glfwSetErrorCallback(GlfwEventHandler::on_error);
    if (glfwInit() == 0) { NOTF_THROW(StartupError, "GLFW initialization failed"); }
    m_state.store(State::READY);

    // default GLFW Window and OpenGL context hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_NONE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config::is_debug_build() ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, config::is_debug_build() ? GLFW_FALSE : GLFW_TRUE);

    // create the shared window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_shared_context.reset(glfwCreateWindow(1, 1, "", nullptr, nullptr));
    if (!m_shared_context) { NOTF_THROW(StartupError, "OpenGL context creation failed."); }
    //    TheGraphicsSystem::Access<Application>::initialize(m_shared_window.get());

    // register other glfw callbacks
    glfwSetMonitorCallback(GlfwEventHandler::on_monitor_change);
    glfwSetJoystickCallback(GlfwEventHandler::on_joystick_change);

    // initialize other members
    m_windows = std::make_unique<std::vector<WindowHandle>>();
    m_scheduler = std::make_unique<Scheduler>();

    // log application header
    NOTF_LOG_INFO("NOTF {} ({} built with {} from {}commit \"{}\")",
                  config::version_string(),                           //
                  (config::is_debug_build() ? "debug" : "release"),   //
                  config::compiler_name(),                            //
                  (config::was_commit_modified() ? "modified " : ""), //
                  config::built_from_commit());
    NOTF_LOG_INFO("GLFW version: {}", glfwGetVersionString());
}

TheApplication::Application::~Application() { _shutdown(); }

int TheApplication::Application::exec() {
    if (State actual = State::READY; !(m_state.compare_exchange_strong(actual, State::RUNNING))) {
        NOTF_THROW(StartupError, "Cannot call `exec` on an already running Application");
    }
    NOTF_LOG_TRACE("Starting main loop");

    // loop until there are no more windows open...
    while (!m_windows->empty()) {

        // ... or the user has initiated a forced shutdown
        if (m_state == State::SHUTDOWN) { break; }

        // wait for the next event or the next time to fire an animation frame
        glfwWaitEvents();
    }

    _shutdown();
    return EXIT_SUCCESS;
}

void TheApplication::Application::shutdown() {
    if (State expected = State::RUNNING; !(m_state.compare_exchange_strong(expected, State::SHUTDOWN))) {
        glfwPostEmptyEvent();
    }
}

void TheApplication::Application::register_window(WindowHandle window) {
    NOTF_ASSERT(!contains(*m_windows, window));

    GLFWwindow* glfw_window = WindowHandle::AccessFor<TheApplication>::get_glfw_window(window);
    m_windows->emplace_back(std::move(window));

    // register all glfw callbacks
    // input callbacks
    glfwSetMouseButtonCallback(glfw_window, GlfwEventHandler::on_mouse_button);
    glfwSetCursorPosCallback(glfw_window, GlfwEventHandler::on_cursor_move);
    glfwSetCursorEnterCallback(glfw_window, GlfwEventHandler::on_cursor_entered);
    glfwSetScrollCallback(glfw_window, GlfwEventHandler::on_scroll);
    glfwSetKeyCallback(glfw_window, GlfwEventHandler::on_token_key);
    glfwSetCharCallback(glfw_window, GlfwEventHandler::on_char_input);
    glfwSetCharModsCallback(glfw_window, GlfwEventHandler::on_shortcut);

    // window callbacks
    glfwSetWindowPosCallback(glfw_window, GlfwEventHandler::on_window_move);
    glfwSetWindowSizeCallback(glfw_window, GlfwEventHandler::on_window_resize);
    glfwSetFramebufferSizeCallback(glfw_window, GlfwEventHandler::on_framebuffer_resize);
    glfwSetWindowRefreshCallback(glfw_window, GlfwEventHandler::on_window_refresh);
    glfwSetWindowFocusCallback(glfw_window, GlfwEventHandler::on_window_focus);
    glfwSetDropCallback(glfw_window, GlfwEventHandler::on_file_drop);
    glfwSetWindowIconifyCallback(glfw_window, GlfwEventHandler::on_window_minimize);
    glfwSetWindowCloseCallback(glfw_window, GlfwEventHandler::on_window_close);
}

void TheApplication::Application::unregister_window(WindowHandle window) {
    // remove the window
    auto itr = std::find(m_windows->begin(), m_windows->end(), window);
    if (itr == m_windows->end()) {
        return;
    } else {
        m_windows->erase(itr);
    }

    // disconnect the window callbacks
    GLFWwindow* glfw_window = WindowHandle::AccessFor<TheApplication>::get_glfw_window(window);
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
}

void TheApplication::Application::_shutdown() {
    State state = m_state;
    if ((state != State::EMPTY) && (m_state.compare_exchange_strong(state, State::EMPTY))) {
        NOTF_LOG_TRACE("Application shutdown");
        m_windows->clear();
        glfwTerminate();
        m_shared_context.reset();
    }
}

// accessors ======================================================================================================== //

void Accessor<TheApplication, Window>::register_window(WindowHandle window) {
    TheApplication::_get().register_window(std::move(window));
}

void Accessor<TheApplication, Window>::unregister_window(WindowHandle window) {
    TheApplication::_get().unregister_window(std::move(window));
}

NOTF_CLOSE_NAMESPACE
