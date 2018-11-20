#include "notf/app/application.hpp"

#include "notf/common/vector.hpp"
#include "notf/common/version.hpp"

#include "notf/app/glfw.hpp"

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

TheApplication::TheApplication(Args args) : m_args(std::move(args)), m_shared_context(nullptr, detail::window_deleter)
{
    // initialize logger first, to catch errors right away
    notf::TheLogger::initialize(m_args.logger_arguments);

    // initialize GLFW
    if (glfwInit() == 0) {
        _shutdown();
        NOTF_THROW(StartupError, "GLFW initialization failed");
    }

    // default GLFW Window and OpenGL context hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    //    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    //    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    //    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    //    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
    //    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_NONE);
    //    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config::is_debug_build() ? GLFW_TRUE : GLFW_FALSE);
    //    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, config::is_debug_build() ? GLFW_FALSE : GLFW_TRUE);

    // create the shared window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_shared_context.reset(glfwCreateWindow(1, 1, "", nullptr, nullptr));
    if (!m_shared_context) { NOTF_THROW(StartupError, "OpenGL context creation failed."); }
    //    TheGraphicsSystem::Access<TheApplication>::initialize(m_shared_window.get());

    // log application header
    NOTF_LOG_INFO("NOTF {} ({} built with {} from {}commit \"{}\")",
                  config::version_string(),                           //
                  (config::is_debug_build() ? "debug" : "release"),   //
                  config::compiler_name(),                            //
                  (config::was_commit_modified() ? "modified " : ""), //
                  config::built_from_commit());
    NOTF_LOG_INFO("GLFW version: {}", glfwGetVersionString());
}

TheApplication::~TheApplication() { _shutdown(); }

int TheApplication::exec()
{
    TheApplication& app = TheApplication::get();

    NOTF_LOG_TRACE("Starting main loop");

    // loop until there are no more windows open
    while (!app.m_windows.empty()) {

        // wait for the next event or the next time to fire an animation frame
        glfwWaitEvents();

        NOTF_LOG_INFO("Jup");
    }

    app._shutdown();
    return EXIT_SUCCESS;
}

void TheApplication::_shutdown()
{
    // you can only close the application once
    if (!is_running()) { return; }
    s_state.store(State::CLOSED);

    // close all remaining windows and their scenes
    for (WindowHandle& window : m_windows) {
        window.close();
    }
    m_windows.clear();
    //    TheGraphicsSystem::Access<TheApplication>::shutdown();
    glfwTerminate();

    // release all resources and objects
    //    m_thread_pool.reset();
    //    m_event_manager.reset();
    //    ResourceManager::get_instance().clear();

    // stop the logger last
    NOTF_LOG_TRACE("Application shutdown");
}

void TheApplication::_register_window(WindowHandle window)
{
    NOTF_ASSERT(!contains(m_windows, window));
    m_windows.emplace_back(std::move(window));
}

void TheApplication::_unregister_window(WindowHandle window)
{
    auto it = std::find(m_windows.begin(), m_windows.end(), window);
    NOTF_ASSERT(it != m_windows.end(), "Cannot remove an unknown Window from the application");
    m_windows.erase(it);
}

NOTF_CLOSE_NAMESPACE
