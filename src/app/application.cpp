#include "notf/app/application.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"
#include "notf/common/version.hpp"

#include "notf/app/glfw.hpp"
#include "notf/app/graph.hpp"
#include "notf/app/timer_pool.hpp"
#include "notf/app/window.hpp"

#include "notf/app/event_handler.hpp"
#include "notf/app/input.hpp"

// glfw event handler =============================================================================================== //

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
        NOTF_LOG_CRIT("GLFW Error \"{}\"({}): {}", error_name, error, message);
    }

    /// Called when the user presses or releases a mouse button Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param button       The mouse button triggering this callback.
    /// @param action       Mouse button action, is either GLFW_PRESS or GLFW_RELEASE.
    /// @param modifiers    Modifier key bitmask.
    static void on_mouse_button(GLFWwindow* glfw_window, const int button, const int action, const int modifiers) {
        //        if (action == GLFW_PRESS) {
        //            schedule<MouseButtonPressEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
        //        } else {
        //            NOTF_ASSERT(action == GLFW_RELEASE);
        //            schedule<MouseButtonReleaseEvent>(glfw_window, MouseButton(button), KeyModifiers(modifiers));
        //        }
    }

    /// Called when the user moves the mouse inside a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            X coordinate of the cursor in Window coordinates.
    /// @param y            Y coordinate of the cursor in Window coordinates.
    static void on_cursor_move(GLFWwindow* glfw_window, const double x, const double y) {
        //        schedule<MouseMoveEvent>(glfw_window, V2d{x, y});
    }

    /// Called when the cursor enters or exists the client area of a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param entered      Is GLFW_TRUE if the cursor entered the window's client area, or GLFW_FALSE if it left it.
    static void on_cursor_entered(GLFWwindow* glfw_window, int entered) {
        //        if (entered == GLFW_TRUE) {
        //            schedule<WindowCursorEnteredEvent>(glfw_window);
        //        } else {
        //            NOTF_ASSERT(entered == GLFW_FALSE);
        //            schedule<WindowCursorExitedEvent>(glfw_window);
        //        }
    }

    /// Called when the user scrolls inside the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            Horizontal scroll delta in screen coordinates.
    /// @param y            Vertical scroll delta in screen coordinates.
    static void on_scroll(GLFWwindow* glfw_window, const double x, const double y) {
        //        schedule<MouseScrollEvent>(glfw_window, V2d{x, y});
    }

    /// Called by GLFW when a key is pressed, repeated or released.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param key          Modified key.
    /// @param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// @param action       The action that triggered this callback.
    /// @param modifiers    Modifier key bitmask.
    static void
    on_token_key(GLFWwindow* glfw_window, const int key, const int scancode, const int action, const int modifiers) {
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

    /// Called by GLFW when an unicode code point was generated.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void on_char_input(GLFWwindow* glfw_window, const uint codepoint) {
        //        schedule<CharInputEvent>(glfw_window, codepoint);
    }

    /// Called by GLFW when the user presses a key combination with at least one modifier key.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void on_shortcut(GLFWwindow* glfw_window, const uint codepoint, const int modifiers) {
        //        schedule<ShortcutEvent>(glfw_window, codepoint, KeyModifiers(modifiers));
    }

    /// Called when the Window was moved.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            The new x-coordinate of the top-left corner of the Window, in screen coordinates.
    /// @param y            The new y-coordinate of the top-left corner of the Window, in screen coordinates.
    static void on_window_move(GLFWwindow* glfw_window, const int x, const int y) {
        //        schedule<WindowMoveEvent>(glfw_window, V2i{x, y});
    }

    /// Called when the Window is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the Window.
    /// @param height       New height of the Window.
    static void on_window_resize(GLFWwindow* glfw_window, const int width, const int height) {
        //        schedule<WindowResizeEvent>(glfw_window, Size2i{width, height});
    }

    /// Called when the Window's framebuffer is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the framebuffer.
    /// @param height       New height of the framebuffer.
    static void on_framebuffer_resize(GLFWwindow* glfw_window, const int width, const int height) {
        //        schedule<WindowResolutionChangeEvent>(glfw_window, Size2i{width, height});
    }

    /// Called when the Window is refreshed by the OS.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    static void on_window_refresh(GLFWwindow* glfw_window) {
        //        schedule<WindowRefreshEvent>(glfw_window);
    }

    /// Called when the Window has gained/lost OS focus.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE if the window gained focus, or GLFW_FALSE if it lost it.
    static void on_window_focus(GLFWwindow* glfw_window, const int kind) {
        //        if (kind == GLFW_TRUE) {
        //            schedule<WindowFocusGainEvent>(glfw_window);
        //        } else {
        //            NOTF_ASSERT(kind == GLFW_FALSE);
        //            schedule<WindowFocusLostEvent>(glfw_window);
        //        }
    }

    /// Called when the Window is minimzed.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE when the Window was minimized, or GLFW_FALSE if it was restored.
    static void on_window_minimize(GLFWwindow* glfw_window, const int kind) {
        //        if (kind == GLFW_TRUE) {
        //            schedule<WindowMinimizeEvent>(glfw_window);
        //        } else {
        //            NOTF_ASSERT(kind == GLFW_FALSE);
        //            schedule<WindowRestoredEvent>(glfw_window);
        //        }
    }

    /// Called by GLFW when the user drops one or more files into the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param count        Number of dropped files.
    /// @param paths        The UTF-8 encoded file and/or directory path names.
    static void on_file_drop(GLFWwindow* glfw_window, const int count, const char** paths) {
        //        schedule<WindowFileDropEvent>(glfw_window, static_cast<uint>(count), paths);
    }

    /// Called by GLFW, if the user requested a window to be closed.
    /// @param glfw_window  GLFW Window to close.
    static void on_window_close(GLFWwindow* glfw_window) {
        // TODO: ask the Window if it should really be closed; ignore and unset the flag if not
        TheEventHandler()->schedule(
            [window = to_window_handle(glfw_window)]() mutable { window->call<Window::to_close>(); });
    }

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

// window deleter =================================================================================================== //

NOTF_OPEN_NAMESPACE

namespace detail {

void window_deleter(GLFWwindow* glfw_window) {
    if (glfw_window != nullptr) { glfwDestroyWindow(glfw_window); }
}

} // namespace detail

// application ====================================================================================================== //

namespace detail {

Application::Application(Arguments args)
    : m_arguments(std::move(args)), m_ui_lock(m_ui_mutex), m_event_queue(m_arguments.app_buffer_size) {
    // initialize GLFW
    glfwSetErrorCallback(GlfwEventHandler::on_error);
    if (glfwInit() == 0) { NOTF_THROW(StartupError, "GLFW initialization failed"); }

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
    m_windows = std::make_unique<decltype(m_windows)::element_type>();

    m_event_handler = TheEventHandler::AccessFor<Application>::create(m_arguments.event_buffer_size);
    NOTF_ASSERT(m_event_handler->is_holder());

    m_timer_pool = TheTimerPool::AccessFor<Application>::create(m_arguments.timer_buffer_size);
    NOTF_ASSERT(m_timer_pool->is_holder());

    m_graph = TheGraph::AccessFor<Application>::create();
    NOTF_ASSERT(m_graph->is_holder());

    // log application header
    NOTF_LOG_INFO("NOTF {} ({} built with {} from {}commit \"{}\")\n"
                  "             GLFW version: {}",
                  config::version_string(),                           //
                  (config::is_debug_build() ? "debug" : "release"),   //
                  config::compiler_name(),                            //
                  (config::was_commit_modified() ? "modified " : ""), //
                  config::built_from_commit(),                        //
                  glfwGetVersionString());
}

Application::~Application() { glfwTerminate(); }

int Application::exec() {
    // make sure exec is only called once
    if (State expected = State::UNSTARTED; !m_state.compare_exchange_strong(expected, State::RUNNING)) {
        NOTF_THROW(StartupError, "Cannot call `exec` on an already running Application");
    }

    // turn the event handler into the UI thread
    NOTF_LOG_INFO("Starting main loop");
    TheEventHandler::AccessFor<Application>::start(m_ui_mutex);
    m_ui_lock.unlock();

    AnyEventPtr event;
    while (m_state.load() == State::RUNNING) {

        // wait for and execute all GLFW events
        glfwWaitEvents();

        { // see if there are any events scheduled to run on the main thread
            while (m_event_queue.try_pop(event) == fibers::channel_op_status::success) {
                event->run();
            }
        }
    }

    // after the event handler is closed, the main thread becomes the UI thread again
    TheEventHandler::AccessFor<Application>::stop();
    m_ui_lock.lock();

    // shutdown
    m_timer_pool.reset();
    m_event_handler.reset();
    m_windows->clear();
    m_shared_context.reset();
    m_graph.reset();
    NOTF_LOG_INFO("Application shutdown");

    return EXIT_SUCCESS;
}

void Application::schedule(AnyEventPtr&& event) {
    m_event_queue.push(std::move(event));
    glfwPostEmptyEvent();
}

void Application::shutdown() {

    if (State expected = State::RUNNING; m_state.compare_exchange_strong(expected, State::CLOSED)) {
        glfwPostEmptyEvent();
    }
}

void Application::_register_window(GLFWwindow* window) {

    schedule([this, window]() mutable {
        { // store the window
            NOTF_GUARD(std::lock_guard(m_windows_mutex));
            detail::GlfwWindowPtr window_ptr(window, detail::window_deleter);
            NOTF_ASSERT(!contains(*m_windows, window_ptr));
            m_windows->emplace_back(std::move(window_ptr));
        }

        // register input callbacks
        glfwSetMouseButtonCallback(window, GlfwEventHandler::on_mouse_button);
        glfwSetCursorPosCallback(window, GlfwEventHandler::on_cursor_move);
        glfwSetCursorEnterCallback(window, GlfwEventHandler::on_cursor_entered);
        glfwSetScrollCallback(window, GlfwEventHandler::on_scroll);
        glfwSetKeyCallback(window, GlfwEventHandler::on_token_key);
        glfwSetCharCallback(window, GlfwEventHandler::on_char_input);
        glfwSetCharModsCallback(window, GlfwEventHandler::on_shortcut);

        // register window callbacks
        glfwSetWindowPosCallback(window, GlfwEventHandler::on_window_move);
        glfwSetWindowSizeCallback(window, GlfwEventHandler::on_window_resize);
        glfwSetFramebufferSizeCallback(window, GlfwEventHandler::on_framebuffer_resize);
        glfwSetWindowRefreshCallback(window, GlfwEventHandler::on_window_refresh);
        glfwSetWindowFocusCallback(window, GlfwEventHandler::on_window_focus);
        glfwSetDropCallback(window, GlfwEventHandler::on_file_drop);
        glfwSetWindowIconifyCallback(window, GlfwEventHandler::on_window_minimize);
        glfwSetWindowCloseCallback(window, GlfwEventHandler::on_window_close);
    });
}

void Application::_unregister_window(GLFWwindow* window) {
    schedule([this, window]() mutable {
        // disconnect all callbacks
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetCursorEnterCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);
        glfwSetKeyCallback(window, nullptr);
        glfwSetCharCallback(window, nullptr);
        glfwSetCharModsCallback(window, nullptr);
        glfwSetWindowPosCallback(window, nullptr);
        glfwSetWindowSizeCallback(window, nullptr);
        glfwSetFramebufferSizeCallback(window, nullptr);
        glfwSetWindowRefreshCallback(window, nullptr);
        glfwSetWindowFocusCallback(window, nullptr);
        glfwSetDropCallback(window, nullptr);
        glfwSetWindowIconifyCallback(window, nullptr);
        glfwSetWindowCloseCallback(window, nullptr);

        { // destroy the window
            NOTF_GUARD(std::lock_guard(m_windows_mutex));
            auto itr = std::find_if(m_windows->begin(), m_windows->end(),
                                    [window](const detail::GlfwWindowPtr& stored) { return stored.get() == window; });
            if (itr != m_windows->end()) {
                m_windows->erase(itr);
                if (m_windows->empty()) { shutdown(); }
            }
        }
    });
}

} // namespace detail

// this_thread (injection) ========================================================================================== //

namespace this_thread {

bool is_the_ui_thread() { return TheApplication()._is_this_the_ui_thread(); }

} // namespace this_thread

NOTF_CLOSE_NAMESPACE
