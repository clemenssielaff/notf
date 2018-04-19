#include "app/application.hpp"

#include <algorithm>

#include "app/event_manager.hpp"
#include "app/glfw.hpp"
#include "app/property_graph.hpp"
#include "app/render_manager.hpp"
#include "app/resource_manager.hpp"
#include "app/timer_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/thread_pool.hpp"

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

application_initialization_error::~application_initialization_error() {}

application_shutdown_error::~application_shutdown_error() {}

//====================================================================================================================//

const Application::Args Application::s_invalid_args;
std::atomic<bool> Application::s_was_closed{false};

Application::Application(const Args& application_args)
    : m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager()
    , m_thread_pool(std::make_unique<ThreadPool>())
    , m_render_manager(std::make_unique<RenderManager>())
    , m_property_graph(std::make_unique<PropertyGraph>())
    , m_event_manager(std::make_unique<EventManager>())
    , m_timer_manager(std::make_unique<TimerManager>())
    , m_windows()
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, m_log_handler.get(), std::placeholders::_1));
    m_log_handler->start();

    // exit here, if the user failed to call Application::initialize()
    if (application_args.argc == -1) {
        notf_throw_format(application_initialization_error, "Cannot start an uninitialized Application!\n"
                                                            "Make sure to call `Application::initialize()` in `main()` "
                                                            "before creating the first NoTF object");
    }

    try { // create the resource manager
        ResourceManager::Args args;
        args.texture_directory = application_args.texture_directory;
        args.fonts_directory = application_args.fonts_directory;
        args.shader_directory = application_args.shader_directory;
        args.executable_path = application_args.argv[0];
        m_resource_manager = std::make_unique<ResourceManager>(std::move(args));
    }
    catch (const resource_manager_initialization_error& error) {
        notf_throw(application_initialization_error, error.what());
    }

    // initialize GLFW
    if (!glfwInit()) {
        _shutdown();
        notf_throw(application_initialization_error, "GLFW initialization failed");
    }
    log_info << "GLFW version: " << glfwGetVersionString();

    // initialize other NoTF mechanisms
    Time::initialize();
}

Application::~Application() { _shutdown(); }

Window& Application::create_window()
{
    m_windows.emplace_back(Window::Access<Application>::create());
    return *m_windows.back().get();
}

Window& Application::create_window(const detail::WindowArguments& args)
{
    m_windows.emplace_back(Window::Access<Application>::create(args));
    return *m_windows.back().get();
}

int Application::exec()
{
    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // wait for the next event or the next time to fire an animation frame
        glfwWaitEvents();
    }

    _shutdown();
    return 0;
}

void Application::_unregister_window(Window* window)
{
    // unregister the window
    auto it = std::find_if(m_windows.begin(), m_windows.end(),
                           [&](const WindowPtr& candiate) -> bool { return candiate.get() == window; });
    NOTF_ASSERT_MSG(it != m_windows.end(), "Cannot remove unknown Window from instance");
    m_windows.erase(it);
}

void Application::_shutdown()
{
    // you can only close the application once
    if (was_closed()) {
        return;
    }

    // close all remaining windows and their scenes
    for (WindowPtr& window : m_windows) {
        window->close();
    }
    m_windows.clear();
    m_render_manager.reset();

    // stop the event loop
    glfwTerminate();

    // by this point, the application is considered "closed"
    s_was_closed.store(true, std::memory_order_release);

    // release all resources and objects
    m_thread_pool.reset();
    m_property_graph.reset();
    m_event_manager.reset();
    m_resource_manager.reset();

    // stop the logger last
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

NOTF_CLOSE_NAMESPACE
