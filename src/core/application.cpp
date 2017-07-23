#include "core/application.hpp"

#include "common/integer.hpp"
#include "common/log.hpp"
#include "common/time.hpp"
#include "common/vector2.hpp"
#include "common/warnings.hpp"
#include "core/events/char_event.hpp"
#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/glfw.hpp"
#include "core/resource_manager.hpp"
#include "core/window.hpp"
#include "utils/print_notf.hpp"

#ifdef NOTF_PYTHON
#include "ext/python/interpreter.hpp"
#endif

namespace { // anonymous

using namespace notf;

/** The current state of all keyboard keys. */
KeyStateSet g_key_states;

/** Currently pressed key modifiers. */
KeyModifiers g_key_modifiers;

/** The current state of all mouse buttons. */
ButtonStateSet g_button_states;

/** Current position of the mouse cursor in screen coordinates. */
Vector2f g_cursor_pos;

/** Previous position of the mouse cursor in screen coordinates. */
Vector2f g_prev_cursor_pos;

} // namespace anonymous

namespace notf {

Application::Application(ApplicationInfo info)
    : m_info(std::move(info))
    , m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager(std::make_unique<ResourceManager>())
    , m_windows()
    , m_current_window()
#ifdef NOTF_PYTHON
    , m_interpreter(nullptr)
#endif
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

    // replace relative (default) paths with absolute ones
    const std::string exe_path = m_info.argv[0];
    const std::string exe_dir  = exe_path.substr(0, exe_path.find_last_of("/") + 1);
    if (m_info.texture_directory.empty() || m_info.texture_directory[0] != '/') {
        m_info.texture_directory = exe_dir + m_info.texture_directory;
    }
    if (m_info.fonts_directory.empty() || m_info.fonts_directory[0] != '/') {
        m_info.fonts_directory = exe_dir + m_info.fonts_directory;
    }
    if (m_info.app_directory.empty() || m_info.app_directory[0] != '/') {
        m_info.app_directory = exe_dir + m_info.app_directory;
    }

    // set resource directories
    if (!m_info.texture_directory.empty()) {
        m_resource_manager->set_texture_directory(m_info.texture_directory);
    }
    if (!m_info.fonts_directory.empty()) {
        m_resource_manager->set_font_directory(m_info.fonts_directory);
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

    // initialize other NoTF mechanisms
    Time::initialize();

#ifdef NOTF_PYTHON
    // initialize Python
    m_interpreter = std::make_unique<PythonInterpreter>(m_info.argv, m_info.app_directory);
#endif

    // vanity plate
    // print_notf();
}

Application::~Application()
{
    _shutdown();
}

int Application::exec()
{
    const Time::Ticks ticks_per_frame = m_info.fps > 0 ? (Time::frequency() / static_cast<Time::Ticks>(m_info.fps)) : 0;
    Time::Ticks next_frame            = next_interval(Time::now().ticks, ticks_per_frame);
    bool fire_animation               = false;

    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // fire the animation signal before redrawing the windows
        if (fire_animation) {
            on_frame();
            fire_animation = false;
        }

        // update all windows
        for (const std::shared_ptr<Window>& window : m_windows) {
            window->_update();
        }

        // find out if you need to fire an animation frame on the next cycle
        const Time now = Time::now();
        if (m_info.fps > 0 && now.ticks >= next_frame) {
            fire_animation = true;
            next_frame     = next_interval(now.ticks, ticks_per_frame);
        }

        //#ifdef _DEBUG
        { // print debug stats
            static Time last_frame    = Time::now();
            static Time time_counter  = 0;
            static uint frame_counter = 0; // actual number of frames in the last second
            static uint anim_counter  = 0; // times the `on_frame` signal was fired in the last second

            time_counter += now.since(last_frame);
            last_frame = now;
            ++frame_counter;
            anim_counter += fire_animation ? 1 : 0;
            if (time_counter >= Time::frequency()) {
                //                log_trace << frame_counter << "fps / " << anim_counter << " animation frames";
                frame_counter = 0;
                anim_counter  = 0;
                time_counter -= Time::frequency();
            }
        }
        //#endif

        // wait for the next event or the next time to fire an animation frame
        if (m_info.fps == 0) {
            glfwWaitEvents();
        }
        else {
            glfwWaitEventsTimeout((next_frame > now.ticks ? next_frame - now.ticks : 0) / static_cast<double>(Time::frequency()));
        }
        //glfwPollEvents();
    }

    _shutdown();
    return to_number(RETURN_CODE::SUCCESS);
}

void Application::_on_error(int error, const char* message)
{
    log_critical << "GLFW Error " << error << ": '" << message << "'";
}

void Application::_on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers)
{
    assert(glfw_window);
    UNUSED(scancode);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // update the global state
    Key notf_key = from_glfw_key(key);
    set_key(g_key_states, notf_key, action);
    g_key_modifiers = KeyModifiers(modifiers);

    // let the window fire the key event
    window->_propagate_key_event(KeyEvent(*window, notf_key, KeyAction(action), KeyModifiers(modifiers), g_key_states));
}

void Application::_on_char_input(GLFWwindow* glfw_window, uint codepoint, int modifiers)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);
    window->_propagate_char_event(CharEvent(*window, codepoint, KeyModifiers(modifiers), g_key_states));
}

void Application::_on_cursor_entered(GLFWwindow* glfw_window, int entered)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // emit the appropriate signal
    if (entered) {
        window->on_cursor_entered(*window);
    }
    else {
        window->on_cursor_exited(*window);
    }
}

void Application::_on_cursor_move(GLFWwindow* glfw_window, double x, double y)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    { // update the global state
        g_prev_cursor_pos = g_cursor_pos;

        Vector2i window_pos;
        glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());
        g_cursor_pos.x() = static_cast<float>(window_pos.x()) + static_cast<float>(x);
        g_cursor_pos.y() = static_cast<float>(window_pos.y()) + static_cast<float>(y);
    }

    // propagate the event
    MouseEvent mouse_event(
        *window,
        {static_cast<float>(x), static_cast<float>(y)}, // event position in window coordinates
        g_cursor_pos - g_prev_cursor_pos,               // delta in window coordinates
        Button::NONE,                                   // move events are triggered by no button
        MouseAction::MOVE,
        g_key_modifiers,
        g_button_states);
    window->_propagate_mouse_event(std::move(mouse_event));
}

void Application::_on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    // parse raw arguments
    Button notf_button      = static_cast<Button>(button);
    MouseAction notf_action = static_cast<MouseAction>(action);
    assert(notf_action == MouseAction::PRESS || notf_action == MouseAction::RELEASE);

    // update the global state
    set_button(g_button_states, notf_button, action);
    g_key_modifiers = KeyModifiers(modifiers);

    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());

    // propagate the event
    MouseEvent mouse_event(
        *window,
        {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
         g_cursor_pos.y() - static_cast<float>(window_pos.y())},
        Vector2f::zero(),
        notf_button,
        notf_action,
        g_key_modifiers,
        g_button_states);
    window->_propagate_mouse_event(std::move(mouse_event));
}

void Application::_on_scroll(GLFWwindow* glfw_window, double x, double y)
{
    assert(glfw_window);
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    assert(window);

    Vector2i window_pos;
    glfwGetWindowPos(glfw_window, &window_pos.x(), &window_pos.y());

    // propagate the event
    MouseEvent mouse_event(
        *window,
        {g_cursor_pos.x() - static_cast<float>(window_pos.x()),
         g_cursor_pos.y() - static_cast<float>(window_pos.y())},
        {static_cast<float>(x), static_cast<float>(y)},
        Button::NONE,
        MouseAction::SCROLL,
        g_key_modifiers, g_button_states);
    window->_propagate_mouse_event(std::move(mouse_event));
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
    window->_on_resize(width, height);
}

void Application::_register_window(std::shared_ptr<Window> window)
{
    assert(window);
    GLFWwindow* glfw_window = window->_get_glfw_window();
    if (!glfw_window) {
        log_fatal << "Window or context creation failed for window '" << window->get_title() << "'";
        _shutdown();
        exit(to_number(RETURN_CODE::GLFW_FAILURE));
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
    GLFWwindow* glfw_window = window->_get_glfw_window();
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
        glfwMakeContextCurrent(window->_get_glfw_window());
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

#ifdef NOTF_PYTHON
    // kill the interpreter
    m_interpreter.reset();
#endif

    // stop the logger
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

} // namespace notf
