#include "app/application.hpp"

#include <algorithm>

#include "app/event_manager.hpp"
#include "app/glfw.hpp"
#include "app/render_manager.hpp"
#include "app/timer_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/resource_manager.hpp"
#include "common/thread_pool.hpp"

namespace {
NOTF_USING_NAMESPACE;
void initialize_resource_types(Application& app);
} // namespace

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Application::initialization_error::~initialization_error() = default;

Application::shut_down_error::~shut_down_error() = default;

// ================================================================================================================== //

std::atomic<bool> Application::s_is_running{true};
const timepoint_t Application::s_start_time = clock_t::now();

Application::Application(Args args)
    : m_args(std::move(args))
    , m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_thread_pool(std::make_unique<ThreadPool>())
    , m_render_manager(std::make_unique<RenderManager>())
    , m_event_manager(std::make_unique<EventManager>())
    , m_timer_manager(std::make_unique<TimerManager>())
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, m_log_handler.get(), std::placeholders::_1));
    m_log_handler->start();

    // exit here, if the user failed to call Application::initialize()
    if (m_args.argc == -1) {
        NOTF_THROW(initialization_error, "Cannot start an uninitialized Application!\n"
                                         "Make sure to call `Application::initialize()` in `main()` "
                                         "before creating the first NoTF object");
    }

    // initialize GLFW
    if (glfwInit() == 0) {
        _shutdown();
        NOTF_THROW(initialization_error, "GLFW initialization failed");
    }
    log_info << "GLFW version: " << glfwGetVersionString();

    initialize_resource_types(*this); // TODO: general purpose callback to call init functions on Application start?
}

Application::~Application() { _shutdown(); }

WindowPtr Application::create_window()
{
    m_windows.emplace_back(Window::Access<Application>::create());
    return m_windows.back();
}

WindowPtr Application::create_window(const detail::WindowSettings& args)
{
    m_windows.emplace_back(Window::Access<Application>::create(args));
    return m_windows.back();
}

int Application::exec()
{
    log_info << "Starting main loop";

    // loop until there are no more windows open
    while (!m_windows.empty()) {

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
    if (!is_running()) {
        return;
    }
    s_is_running.store(false);

    // close all remaining windows and their scenes
    for (WindowPtr& window : m_windows) {
        window->close();
    }
    m_windows.clear();
    m_render_manager.reset();

    glfwTerminate();

    // release all resources and objects
    m_thread_pool.reset();
    m_event_manager.reset();
    ResourceManager::get_instance().clear();

    // stop the logger last
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

NOTF_CLOSE_NAMESPACE

// ================================================================================================================== //

#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/text/font.hpp"

namespace {
NOTF_USING_NAMESPACE;

void initialize_resource_types(Application& app)
{
    ResourceManager& resource_manager = ResourceManager::get_instance();
    const Application::Args& args = app.get_arguments();

    std::string executable_path = args.argv[0];
    executable_path = executable_path.substr(0, executable_path.find_last_of('/') + 1);
    resource_manager.set_base_path(executable_path + args.resource_directory);

    { // Vertex Shader
        auto& resource_type = resource_manager.get_type<VertexShader>();
        resource_type.set_path(args.shader_directory);
    }
    { // Tesselation Shader
        auto& resource_type = resource_manager.get_type<TesselationShader>();
        resource_type.set_path(args.shader_directory);
    }
    { // Geometry Shader
        auto& resource_type = resource_manager.get_type<GeometryShader>();
        resource_type.set_path(args.shader_directory);
    }
    { // Fragment Shader
        auto& resource_type = resource_manager.get_type<FragmentShader>();
        resource_type.set_path(args.shader_directory);
    }

    { // Texture
        auto& resource_type = resource_manager.get_type<Texture>();
        resource_type.set_path(args.texture_directory);
    }

    { // Font
        auto& resource_type = resource_manager.get_type<Font>();
        resource_type.set_path(args.fonts_directory);
    }
}

} // namespace
