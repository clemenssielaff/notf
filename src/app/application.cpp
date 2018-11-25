#include "notf/app/application.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/vector.hpp"
#include "notf/common/version.hpp"

#include "notf/app/glfw.hpp"

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

TheApplication::Application::Application(Arguments args)
    : m_arguments(std::move(args)), m_shared_context(nullptr, detail::window_deleter) {
    // initialize GLFW
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
    while (!m_windows.empty()) {

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
    NOTF_ASSERT(!contains(m_windows, window));
    m_windows.emplace_back(std::move(window));
}

void TheApplication::Application::unregister_window(WindowHandle window) {
    if (auto it = std::find(m_windows.begin(), m_windows.end(), window); it != m_windows.end()) { m_windows.erase(it); }
}

void TheApplication::Application::_shutdown() {
    State state = m_state;
    if ((state != State::EMPTY) && (m_state.compare_exchange_strong(state, State::EMPTY))) {
        NOTF_LOG_TRACE("Application shutdown");
        m_windows.clear();
        glfwTerminate();
        m_shared_context.reset();
    }
}

NOTF_CLOSE_NAMESPACE
