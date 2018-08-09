#include "app/application.hpp"

#include <algorithm>

#include "app/event_manager.hpp"
#include "app/glfw.hpp"
#include "app/render_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/resource_manager.hpp"
#include "common/thread_pool.hpp"
#include "graphics/graphics_system.hpp"

namespace {
NOTF_USING_NAMESPACE;
void initialize_resource_types(TheApplication& app);
} // namespace

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace detail {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window != nullptr) {
        glfwDestroyWindow(glfw_window);
    }
}

} // namespace detail

// ================================================================================================================== //

TheApplication::initialization_error::~initialization_error() = default;

TheApplication::shut_down_error::~shut_down_error() = default;

// ================================================================================================================== //

std::atomic<bool> TheApplication::s_is_running{true};
const timepoint_t TheApplication::s_start_time = clock_t::now();

TheApplication::TheApplication(Args args)
    : m_args(std::move(args))
    , m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_shared_window(nullptr, detail::window_deleter)
    , m_thread_pool(std::make_unique<ThreadPool>())
    , m_render_manager(std::make_unique<RenderManager>())
    , m_event_manager(std::make_unique<EventManager>())
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
    //    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, is_debug_build() ? GLFW_TRUE : GLFW_FALSE);
    //    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, is_debug_build() ? GLFW_FALSE : GLFW_TRUE);

    // create the shared window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_shared_window.reset(glfwCreateWindow(1, 1, "", nullptr, nullptr));
    if (!m_shared_window) {
        NOTF_THROW(initialization_error, "OpenGL context creation failed.");
    }
    TheGraphicsSystem::Access<TheApplication>::initialize(m_shared_window.get());

    initialize_resource_types(*this); // TODO: general purpose callback to call init functions on Application start?
}

TheApplication::~TheApplication() { _shutdown(); }

WindowPtr TheApplication::create_window()
{
    m_windows.emplace_back(Window::Access<TheApplication>::create());
    return m_windows.back();
}

WindowPtr TheApplication::create_window(const detail::WindowSettings& args)
{
    m_windows.emplace_back(Window::Access<TheApplication>::create(args));
    return m_windows.back();
}

int TheApplication::exec()
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

void TheApplication::_unregister_window(Window* window)
{
    // unregister the window
    auto it = std::find_if(m_windows.begin(), m_windows.end(),
                           [&](const WindowPtr& candiate) -> bool { return candiate.get() == window; });
    NOTF_ASSERT_MSG(it != m_windows.end(), "Cannot remove unknown Window from instance");
    m_windows.erase(it);
}

void TheApplication::_shutdown()
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
    TheGraphicsSystem::Access<TheApplication>::shutdown();
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

#include "graphics/shader.hpp"
#include "graphics/text/font.hpp"
#include "graphics/texture.hpp"

namespace {
NOTF_USING_NAMESPACE;

void initialize_resource_types(TheApplication& app)
{
    ResourceManager& resource_manager = ResourceManager::get_instance();
    const TheApplication::Args& args = app.get_arguments();

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
