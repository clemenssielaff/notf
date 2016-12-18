#include "core/application.hpp"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>

#include "common/input.hpp"
#include "common/log.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/item.hpp"
#include "core/resource_manager.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"
#include "utils/unused.hpp"

namespace { // anonymous

using namespace notf;

/** The current state of all keyboard keys. */
KeyStateSet g_key_states;

/** Currently pressed key modifiers. */
KeyModifiers g_key_modifiers;

/** The current state of all mouse buttons. */
ButtonStateSet g_button_states;

/** Last recorded position of the mouse (relative to the last focused Window). */
Vector2 g_mouse_pos = Vector2::zero();

} // namespace anonymous

namespace notf {

Application::Application(const ApplicationInfo info)
    : m_info(std::move(info))
    , m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager(std::make_unique<ResourceManager>())
    , m_windows()
    , m_interpreter(nullptr)
    , m_current_window()
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, m_log_handler.get(), std::placeholders::_1));
    m_log_handler->start();

    // exit here, if the user failed to call Application::initialize()
    if (m_info.argc == -1) {
        log_fatal << "Cannot start an uninitialized Application!\n"
                  << "Make sure to call `Application::initialize()` in `main()` before creating the first NoTF object";
        exit(to_number(RETURN_CODE::UNINITIALIZED));
    }

    // set resource directories
    if (!info.texture_directory.empty()) {
        m_resource_manager->set_texture_directory(info.texture_directory);
    }
    if (!info.fonts_directory.empty()) {
        m_resource_manager->set_font_directory(info.fonts_directory);
    }

    // set the error callback to catch all GLFW errors
    glfwSetErrorCallback(Application::_on_error);

    // initialize GLFW
    if (!glfwInit()) {
        log_fatal << "GLFW initialization failed";
        _shutdown();
        exit(to_number(RETURN_CODE::GLFW_FAILURE));
    }
    log_info << "GLFW version: " << glfwGetVersionString();

    // initialize the time
    Time::set_frequency(glfwGetTimerFrequency());
    glfwSetTime(0);

    // initialize Python
    if (info.enable_python) {
        m_interpreter = std::make_unique<PythonInterpreter>(m_info.argv, m_info.app_directory);
    }

    log_trace << "Started application";
}

Application::~Application()
{
    _shutdown();
}

int Application::exec()
{
    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (const std::shared_ptr<Window>& window : m_windows) {
            window->_update();
        }

        glfwWaitEvents();
    }

    _shutdown();
    return to_number(RETURN_CODE::SUCCESS);
}

Application& Application::initialize(ApplicationInfo& info)
{
    if (info.argc == -1 || info.argv == nullptr) {
        log_fatal << "Cannot initialize an Application from a Info object with missing `argc` and `argv` fields";
        exit(to_number(RETURN_CODE::UNINITIALIZED));
    }
    const std::string exe_path = info.argv[0];
    const std::string exe_dir = exe_path.substr(0, exe_path.find_last_of("/") + 1);

    // replace relative (default) paths with absolute ones
    if (info.texture_directory.empty() || info.texture_directory[0] != '/') {
        info.texture_directory = exe_dir + info.texture_directory;
    }
    if (info.fonts_directory.empty() || info.fonts_directory[0] != '/') {
        info.fonts_directory = exe_dir + info.fonts_directory;
    }
    if (info.app_directory.empty() || info.app_directory[0] != '/') {
        info.app_directory = exe_dir + info.app_directory;
    }
    return _get_instance(info);
}

void Application::_on_error(int error, const char* message)
{
    std::cerr << "GLFW Error " << error << ": '" << message << "'" << std::endl;
}

void Application::_on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers)
{
    assert(glfw_window);
    UNUSED(scancode);
    Window* window_raw = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window_raw);
    std::shared_ptr<Window> window = window_raw->shared_from_this();
    if (!window) {
        log_critical << "Received 'on_token_key' Callback for unknown GLFW window";
        return;
    }

    // update the global state
    Key notf_key = from_glfw_key(key);
    set_key(g_key_states, notf_key, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // let the window fire the key event
    KeyEvent key_event(window.get(), notf_key, KeyAction(action), KeyModifiers(modifiers), g_key_states);
    window->on_token_key(key_event);
}

void Application::_on_cursor_move(GLFWwindow* glfw_window, double x, double y)
{
    assert(glfw_window);
    Window* window_raw = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window_raw);
    std::shared_ptr<Window> window = window_raw->shared_from_this();
    if (!window) {
        log_critical << "Received 'on_token_key' Callback for unknown GLFW window";
        return;
    }

    // update the global state
    g_mouse_pos = {static_cast<float>(x), static_cast<float>(y)};

    // propagate the event
    MouseEvent mouse_event(window.get(), g_mouse_pos, Button::NONE, MouseAction::MOVE, g_key_modifiers, g_button_states);
    window->_propagate_mouse_event(std::move(mouse_event));
}

void Application::_on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers)
{
    assert(glfw_window);
    Window* window_raw = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window_raw);
    std::shared_ptr<Window> window = window_raw->shared_from_this();
    if (!window) {
        log_critical << "Received '_on_mouse_button' Callback for unknown GLFW window";
        return;
    }

    // parse raw arguments
    Button notf_button = static_cast<Button>(button);
    MouseAction notf_action = static_cast<MouseAction>(action);
    assert(notf_action != MouseAction::MOVE);

    // update the global state
    set_button(g_button_states, notf_button, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // propagate the event
    MouseEvent mouse_event(window.get(), g_mouse_pos, notf_button, notf_action, g_key_modifiers, g_button_states);
    window->_propagate_mouse_event(std::move(mouse_event));
}

void Application::_on_window_close(GLFWwindow* glfw_window)
{
    assert(glfw_window);
    Window* window_raw = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window_raw);
    std::shared_ptr<Window> window = window_raw->shared_from_this();
    if (!window) {
        log_critical << "Callback for unknown GLFW window";
        return;
    }
    window->close();
}

void Application::_on_window_reize(GLFWwindow* glfw_window, int width, int height)
{
    assert(glfw_window);
    Window* window_raw = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window_raw);
    std::shared_ptr<Window> window = window_raw->shared_from_this();
    if (!window) {
        log_critical << "Callback for unknown GLFW window";
        return;
    }
    window->_on_resize(width, height);
}

void Application::_register_window(std::shared_ptr<Window> window)
{
    assert(window);
    GLFWwindow* glfw_window = window->_get_glwf_window();
    if (!glfw_window) {
        log_fatal << "Window or context creation failed for window '" << window->get_title() << "'";
        _shutdown();
        exit(to_number(RETURN_CODE::GLFW_FAILURE));
    }
    assert(std::find(m_windows.begin(), m_windows.end(), window) == m_windows.end());

    // register the window
    m_windows.push_back(window);

    // connect the window callbacks
    glfwSetWindowCloseCallback(glfw_window, _on_window_close);
    glfwSetKeyCallback(glfw_window, _on_token_key);
    glfwSetCursorPosCallback(glfw_window, _on_cursor_move);
    glfwSetMouseButtonCallback(glfw_window, _on_mouse_button);
    glfwSetWindowSizeCallback(glfw_window, _on_window_reize);

    // if this is the first Window, it is also the current one
    if (!m_current_window) {
        m_current_window = window;
    }
}

void Application::_unregister_window(std::shared_ptr<Window> window)
{
    assert(window);
    auto iterator = std::find(m_windows.begin(), m_windows.end(), window);
    assert(iterator != m_windows.end());

    // disconnect the window callbacks
    GLFWwindow* glfw_window = window->_get_glwf_window();
    assert(glfw_window);
    glfwSetWindowCloseCallback(glfw_window, nullptr);
    glfwSetKeyCallback(glfw_window, nullptr);
    glfwSetWindowSizeCallback(glfw_window, nullptr);

    // unregister the window
    m_windows.erase(iterator);
}

void Application::_set_current_window(Window* window)
{
    assert(window);
    if (m_current_window.get() != window) {
        glfwMakeContextCurrent(window->_get_glwf_window());
        m_current_window = window->shared_from_this();
    }
}

void Application::_shutdown()
{
    static bool is_running = true;
    if (!is_running) {
        return;
    }
    is_running = false;

    // close all remaining windows
    for (std::shared_ptr<Window>& window : m_windows) {
        window->close();
    }

    // release all resources and objects
    m_resource_manager->clear();

    // stop the event loop
    glfwTerminate();

    // kill the interpreter
    m_interpreter.reset();

    // stop the logger
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

} // namespace notf
