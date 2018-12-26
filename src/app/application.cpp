#include "notf/app/application.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"

#include "notf/app/event_handler.hpp"
#include "notf/app/glfw_callbacks.hpp"
#include "notf/app/graph/graph.hpp"
#include "notf/app/render_manager.hpp"
#include "notf/app/timer_pool.hpp"

#include "notf/graphic/glfw.hpp"
#include "notf/graphic/graphics_system.hpp"

// window deleter =================================================================================================== //

NOTF_OPEN_NAMESPACE

namespace detail {

void window_deleter(GLFWwindow* glfw_window) {
    NOTF_ASSERT(this_thread::is_the_main_thread());
    if (glfw_window != nullptr) { glfwDestroyWindow(glfw_window); }
}

} // namespace detail

// application ====================================================================================================== //

namespace detail {

Application::Application(Arguments args)
    : m_arguments(std::move(args)), m_ui_lock(m_ui_mutex), m_event_queue(m_arguments.app_buffer_size) {
    // initialize GLFW
    glfwSetErrorCallback(GlfwCallbacks::_on_error);
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

    // create the shared context
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_shared_context.reset(glfwCreateWindow(1, 1, "", nullptr, nullptr));
    if (!m_shared_context) { NOTF_THROW(StartupError, "OpenGL context creation failed."); }
    m_graphics_system = TheGraphicsSystem::AccessFor<Application>::create(m_shared_context.get());

    // register other glfw callbacks
    glfwSetMonitorCallback(GlfwCallbacks::_on_monitor_change);
    glfwSetJoystickCallback(GlfwCallbacks::_on_joystick_change);

    // initialize other members
    m_windows = std::make_unique<decltype(m_windows)::element_type>();

    m_event_handler = TheEventHandler::AccessFor<Application>::create(m_arguments.event_buffer_size);
    NOTF_ASSERT(m_event_handler->is_holder());

    m_timer_pool = TheTimerPool::AccessFor<Application>::create(m_arguments.timer_buffer_size);
    NOTF_ASSERT(m_timer_pool->is_holder());

    m_render_manager = TheRenderManager::AccessFor<Application>::create();
    NOTF_ASSERT(m_render_manager->is_holder());

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

Application::~Application() {
    m_graphics_system.reset();
    glfwTerminate();
}

int Application::exec() {
    if (!this_thread::is_the_main_thread()) {
        NOTF_THROW(StartupError, "You must call `Application::exec` only from the main thread");
    }

    // make sure exec is only called once
    if (State expected = State::UNSTARTED; !m_state.compare_exchange_strong(expected, State::RUNNING)) {
        NOTF_ASSERT(expected == State::CLOSED); // RUNNING shouldn't be possible
        NOTF_THROW(StartupError, "Cannot call `exec` on an already closed Application");
    }

    if (!m_windows->empty() || get_arguments().start_without_windows) {

        // turn the event handler into the UI thread
        NOTF_LOG_INFO("Starting main loop");
        TheEventHandler::AccessFor<Application>::start(m_ui_mutex);
        m_ui_lock.unlock();

        AnyAppEventPtr event;
        while (m_state.load() == State::RUNNING) {

            // wait for and execute all GLFW events
            glfwWaitEvents();

            { // see if there are any events scheduled to run on the main thread
                while (m_event_queue.try_pop(event) == fibers::channel_op_status::success
                       && m_state.load() == State::RUNNING) {
                    event->run();
                }
            }
        }

        // close the timer pool first
        // all active timers will be interrupted, unless their "keep-alive" flag is set, which means that the user
        // intended them to block shutdown until they had time to finish
        TheTimerPool()->close();

        // after the event handler is closed, the main thread becomes the UI thread again
        m_event_handler.reset(); // by deleting the event handler we wait for its thread to join, before re-acquiring
        m_ui_lock.lock();        // the ui mutex (otherwise the event handler might wait and block forever)
    }
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    // shutdown in reverse order
    m_render_manager.reset();
    m_graph.reset();
    m_timer_pool.reset();
    m_event_handler.reset();
    m_windows->clear();
    m_graphics_system.reset();
    m_shared_context.reset();
    NOTF_LOG_INFO("Application shutdown");

    return EXIT_SUCCESS;
}

void Application::schedule(AnyAppEventPtr&& event) {
    if (m_state.load() != State::CLOSED) { // you can pre-schedule events prior to startup, but not after shutdown
        m_event_queue.push(std::move(event));
        glfwPostEmptyEvent();
    }
}

void Application::shutdown() {
    State expected = State::RUNNING;
    m_state.compare_exchange_strong(expected, State::CLOSED);
}

void Application::_register_window(GLFWwindow* window) {
    if (!this_thread::is_the_ui_thread()) { NOTF_THROW(ThreadError, "Windows can only be created on the UI thread"); }

    // store the window
    detail::GlfwWindowPtr window_ptr(window, detail::window_deleter);
    NOTF_ASSERT(!contains(*m_windows, window_ptr));
    m_windows->emplace_back(std::move(window_ptr));

    // register all callbacks
    schedule([window]() mutable {
        // input callbacks
        glfwSetMouseButtonCallback(window, GlfwCallbacks::_on_mouse_button);
        glfwSetCursorPosCallback(window, GlfwCallbacks::_on_cursor_move);
        glfwSetCursorEnterCallback(window, GlfwCallbacks::_on_cursor_entered);
        glfwSetScrollCallback(window, GlfwCallbacks::_on_scroll);
        glfwSetKeyCallback(window, GlfwCallbacks::_on_token_key);
        glfwSetCharCallback(window, GlfwCallbacks::_on_char_input);
        glfwSetCharModsCallback(window, GlfwCallbacks::_on_shortcut);

        // window callbacks
        glfwSetWindowPosCallback(window, GlfwCallbacks::_on_window_move);
        glfwSetWindowSizeCallback(window, GlfwCallbacks::_on_window_resize);
        glfwSetFramebufferSizeCallback(window, GlfwCallbacks::_on_framebuffer_resize);
        glfwSetWindowRefreshCallback(window, GlfwCallbacks::_on_window_refresh);
        glfwSetWindowFocusCallback(window, GlfwCallbacks::_on_window_focus);
        glfwSetDropCallback(window, GlfwCallbacks::_on_file_drop);
        glfwSetWindowIconifyCallback(window, GlfwCallbacks::_on_window_minimize);
        glfwSetWindowCloseCallback(window, GlfwCallbacks::_on_window_close);
    });
}

void Application::_unregister_window(GLFWwindow* window) {
    schedule([this, window]() {
        auto itr = std::find_if(m_windows->begin(), m_windows->end(),
                                [window](const detail::GlfwWindowPtr& stored) { return stored.get() == window; });
        if (itr != m_windows->end()) {
            m_windows->erase(itr);
            if (m_windows->empty()) { shutdown(); }
        }
    });
}

} // namespace detail

// this_thread (injection) ========================================================================================== //

namespace this_thread {

bool is_the_ui_thread() { return TheApplication()._is_this_the_ui_thread(); }

} // namespace this_thread

NOTF_CLOSE_NAMESPACE
