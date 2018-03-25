#include "app/application.hpp"

#include <algorithm>

#include "app/glfw.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/keyboard.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/time.hpp"
#include "app/io/window_event.hpp"
#include "app/property_graph.hpp"
#include "app/resource_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "common/thread_pool.hpp"
#include "common/vector2.hpp"

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

application_initialization_error::~application_initialization_error() {}

//====================================================================================================================//

const Application::Args Application::s_invalid_args;

Application::Application(const Args& application_args)
    : m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager()
    , m_thread_pool(std::make_unique<ThreadPool>())
    , m_property_graph(std::make_unique<PropertyGraph>())
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

    // set the error callback to catch all GLFW errors
    glfwSetErrorCallback(Application::_on_error);

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

void Application::_on_error(int error_number, const char* message)
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

void Application::_on_token_key(GLFWwindow* glfw_window, int key, NOTF_UNUSED int scancode, int action, int modifiers)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // update the global state
    Key notf_key = from_glfw_key(key);
    set_key(g_key_states, notf_key, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // let the window handle the event
    Window::Access<Application>(*window).propagate(
        KeyEvent(*window, notf_key, KeyAction(action), KeyModifiers(modifiers), g_key_states));
}

void Application::_on_char_input(GLFWwindow* glfw_window, uint codepoint, int modifiers)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // let the window handle the event
    Window::Access<Application>(*window).propagate(
        CharEvent(*window, codepoint, KeyModifiers(modifiers), g_key_states));
}

void Application::_on_cursor_entered(GLFWwindow* glfw_window, int entered)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    if (entered) {
        Window::Access<Application>(*window).propagate(WindowEvent(*window, WindowEvent::Type::CURSOR_ENTERED));
    }
    else {
        Window::Access<Application>(*window).propagate(WindowEvent(*window, WindowEvent::Type::CURSOR_EXITED));
    }
}

void Application::_on_cursor_move(GLFWwindow* glfw_window, double x, double y)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    { // update the global state
        g_prev_cursor_pos = g_cursor_pos;

        // invert the y-coordinate (by default, y grows down)
        const int window_height = window->window_size().height;
        y = window_height - y;
        Vector2i window_pos;
        glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
        window_pos.y() = window_height - window_pos.y();

        g_cursor_pos.x() = static_cast<float>(window_pos.x()) + static_cast<float>(x);
        g_cursor_pos.y() = static_cast<float>(window_pos.y()) + static_cast<float>(y);
    }

    // let the window handle the event
    MouseEvent mouse_event(*window,
                           {static_cast<float>(x), static_cast<float>(y)}, // event position in window coordinates
                           g_cursor_pos - g_prev_cursor_pos,               // delta in window coordinates
                           Button::NO_BUTTON,                              // move events are triggered by no button
                           MouseAction::MOVE, g_key_modifiers, g_button_states);
    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void Application::_on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // parse raw arguments
    Button notf_button = static_cast<Button>(button);
    MouseAction notf_action = static_cast<MouseAction>(action);
    assert(notf_action == MouseAction::PRESS || notf_action == MouseAction::RELEASE);

    // update the global state
    set_button(g_button_states, notf_button, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // invert the y-coordinate (by default, y grows down)
    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
    window_pos.y() = window->window_size().height - window_pos.y();

    // let the window handle the event
    MouseEvent mouse_event(*window,
                           {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
                            g_cursor_pos.y() - static_cast<float>(window_pos.y())},
                           Vector2f::zero(), notf_button, notf_action, g_key_modifiers, g_button_states);
    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void Application::_on_scroll(GLFWwindow* glfw_window, double x, double y)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // invert the y-coordinate (by default, y grows down)
    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
    window_pos.y() = window->window_size().height - window_pos.y();

    // let the window handle the event
    MouseEvent mouse_event(*window,
                           {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
                            g_cursor_pos.y() - static_cast<float>(window_pos.y())},
                           {static_cast<float>(x), static_cast<float>(-y)}, Button::NO_BUTTON, MouseAction::SCROLL,
                           g_key_modifiers, g_button_states);
    Window::Access<Application>(*window).propagate(std::move(mouse_event));
}

void Application::_on_window_close(GLFWwindow* glfw_window)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);
    window->close();
}

void Application::_on_window_resize(GLFWwindow* glfw_window, int width, int height)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);
    Window::Access<Application>(*window).resize({width, height});
}

void Application::_register_window(const WindowPtr& window)
{
    assert(window);
    GLFWwindow* glfw_window = Window::Access<Application>(*window).glfw_window();
    if (!glfw_window) {
        _shutdown();
        notf_throw_format(window_initialization_error,
                          "Window or context creation failed for window '" << window->title() << "'");
    }
    assert(std::find(m_windows.begin(), m_windows.end(), window) == m_windows.end());

    // register the window
    m_windows.push_back(window);

    // connect the window callbacks
    glfwSetKeyCallback(glfw_window, _on_token_key);
    glfwSetCharModsCallback(glfw_window, _on_char_input);

    glfwSetCursorEnterCallback(glfw_window, _on_cursor_entered);
    glfwSetCursorPosCallback(glfw_window, _on_cursor_move);
    glfwSetMouseButtonCallback(glfw_window, _on_mouse_button);
    glfwSetScrollCallback(glfw_window, _on_scroll);

    glfwSetWindowCloseCallback(glfw_window, _on_window_close);
    glfwSetWindowSizeCallback(glfw_window, _on_window_resize);
}

void Application::_unregister_window(Window* window)
{
    assert(window);

    // disconnect the window callbacks
    GLFWwindow* glfw_window = Window::Access<Application>(*window).glfw_window();
    assert(glfw_window);
    glfwSetWindowCloseCallback(glfw_window, nullptr);
    glfwSetKeyCallback(glfw_window, nullptr);
    glfwSetWindowSizeCallback(glfw_window, nullptr);

    // unregister the window
    auto it = std::find_if(m_windows.begin(), m_windows.end(),
                           [&](const WindowPtr& candiate) -> bool { return candiate.get() == window; });
    assert(it != m_windows.end());
    m_windows.erase(it);
}

void Application::_shutdown()
{
    // you can only close the application once
    static bool is_running = true;
    if (!is_running) {
        return;
    }
    is_running = false;

    // close all remaining windows
    for (WindowPtr& window : m_windows) {
        window->close();
    }
    m_windows.clear();

    // release all resources and objects
    m_resource_manager->clear();

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

NOTF_CLOSE_NAMESPACE
