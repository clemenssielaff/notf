#include "core/application.hpp"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>

#include "common/keyboard.hpp"
#include "common/log.hpp"
#include "common/time.hpp"
#include "core/events/key_event.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/layout_item.hpp"
#include "core/object_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/window.hpp"
#include "python/interpreter.hpp"
#include "utils/unused.hpp"

namespace { // anonymous

/// @brief The current state of all keyboard keys.
/// Is unit-global instead of a member variable to keep the application header small.
notf::KeyStateSet g_key_states;

} // namespace anonymous

namespace notf {

KEY from_glfw_key(int key);

Application::Application(const ApplicationInfo info)
    : m_info(std::move(info))
    , m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager(std::make_unique<ResourceManager>())
    , m_object_manager(std::make_unique<ObjectManager>(1024)) // reserve space for 1024 Items right away
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
    //    double last_time = glfwGetTime();
    //    uint frame_count = 0;

    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (const std::shared_ptr<Window>& window : m_windows) {
            window->_update();
        }

        // poll and process GLWF events
        glfwWaitEvents();
        //        glfwPollEvents();

        //        // print time per frame in ms, averaged over the last second
        //        ++frame_count;
        //        if (glfwGetTime() - last_time >= 1.) {
        //            double ms_per_frame = 1000. / static_cast<double>(frame_count);
        //            log_trace << ms_per_frame << "ms/frame "
        //                      << "(" << static_cast<float>(ms_per_frame / (1. / 6.)) << "%) = "
        //                      << frame_count << "fps";
        //            frame_count = 0;
        //            last_time += 1.;
        //        }
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

    // update the key state
    KEY notf_key = from_glfw_key(key);
    set_key(g_key_states, notf_key, action);

    // let the window fire the key event
    KeyEvent key_event{window.get(), notf_key, KEY_ACTION(action), KEY_MODIFIERS(modifiers), g_key_states};
    window->on_token_key(key_event);
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

KEY from_glfw_key(int key)
{
    switch (key) {
    case GLFW_KEY_SPACE:
        return KEY::SPACE;
    case GLFW_KEY_APOSTROPHE:
        return KEY::APOSTROPHE;
    case GLFW_KEY_COMMA:
        return KEY::COMMA;
    case GLFW_KEY_MINUS:
        return KEY::MINUS;
    case GLFW_KEY_PERIOD:
        return KEY::PERIOD;
    case GLFW_KEY_SLASH:
        return KEY::SLASH;
    case GLFW_KEY_0:
        return KEY::ZERO;
    case GLFW_KEY_1:
        return KEY::ONE;
    case GLFW_KEY_2:
        return KEY::TWO;
    case GLFW_KEY_3:
        return KEY::THREE;
    case GLFW_KEY_4:
        return KEY::FOUR;
    case GLFW_KEY_5:
        return KEY::FIVE;
    case GLFW_KEY_6:
        return KEY::SIX;
    case GLFW_KEY_7:
        return KEY::SEVEN;
    case GLFW_KEY_8:
        return KEY::EIGHT;
    case GLFW_KEY_9:
        return KEY::NINE;
    case GLFW_KEY_SEMICOLON:
        return KEY::SEMICOLON;
    case GLFW_KEY_EQUAL:
        return KEY::EQUAL;
    case GLFW_KEY_A:
        return KEY::A;
    case GLFW_KEY_B:
        return KEY::B;
    case GLFW_KEY_C:
        return KEY::C;
    case GLFW_KEY_D:
        return KEY::D;
    case GLFW_KEY_E:
        return KEY::E;
    case GLFW_KEY_F:
        return KEY::F;
    case GLFW_KEY_G:
        return KEY::G;
    case GLFW_KEY_H:
        return KEY::H;
    case GLFW_KEY_I:
        return KEY::I;
    case GLFW_KEY_J:
        return KEY::J;
    case GLFW_KEY_K:
        return KEY::K;
    case GLFW_KEY_L:
        return KEY::L;
    case GLFW_KEY_M:
        return KEY::M;
    case GLFW_KEY_N:
        return KEY::N;
    case GLFW_KEY_O:
        return KEY::O;
    case GLFW_KEY_P:
        return KEY::P;
    case GLFW_KEY_Q:
        return KEY::Q;
    case GLFW_KEY_R:
        return KEY::R;
    case GLFW_KEY_S:
        return KEY::S;
    case GLFW_KEY_T:
        return KEY::T;
    case GLFW_KEY_U:
        return KEY::U;
    case GLFW_KEY_V:
        return KEY::V;
    case GLFW_KEY_W:
        return KEY::W;
    case GLFW_KEY_X:
        return KEY::X;
    case GLFW_KEY_Y:
        return KEY::Y;
    case GLFW_KEY_Z:
        return KEY::Z;
    case GLFW_KEY_LEFT_BRACKET:
        return KEY::LEFT_BRACKET;
    case GLFW_KEY_BACKSLASH:
        return KEY::BACKSLASH;
    case GLFW_KEY_RIGHT_BRACKET:
        return KEY::RIGHT_BRACKET;
    case GLFW_KEY_GRAVE_ACCENT:
        return KEY::GRAVE_ACCENT;
    case GLFW_KEY_WORLD_1:
        return KEY::WORLD_1;
    case GLFW_KEY_WORLD_2:
        return KEY::WORLD_2;
    case GLFW_KEY_ESCAPE:
        return KEY::ESCAPE;
    case GLFW_KEY_ENTER:
        return KEY::ENTER;
    case GLFW_KEY_TAB:
        return KEY::TAB;
    case GLFW_KEY_BACKSPACE:
        return KEY::BACKSPACE;
    case GLFW_KEY_INSERT:
        return KEY::INSERT;
    case GLFW_KEY_DELETE:
        return KEY::DELETE;
    case GLFW_KEY_RIGHT:
        return KEY::RIGHT;
    case GLFW_KEY_LEFT:
        return KEY::LEFT;
    case GLFW_KEY_DOWN:
        return KEY::DOWN;
    case GLFW_KEY_UP:
        return KEY::UP;
    case GLFW_KEY_PAGE_UP:
        return KEY::PAGE_UP;
    case GLFW_KEY_PAGE_DOWN:
        return KEY::PAGE_DOWN;
    case GLFW_KEY_HOME:
        return KEY::HOME;
    case GLFW_KEY_END:
        return KEY::END;
    case GLFW_KEY_CAPS_LOCK:
        return KEY::CAPS_LOCK;
    case GLFW_KEY_SCROLL_LOCK:
        return KEY::SCROLL_LOCK;
    case GLFW_KEY_NUM_LOCK:
        return KEY::NUM_LOCK;
    case GLFW_KEY_PRINT_SCREEN:
        return KEY::PRINT_SCREEN;
    case GLFW_KEY_PAUSE:
        return KEY::PAUSE;
    case GLFW_KEY_F1:
        return KEY::F1;
    case GLFW_KEY_F2:
        return KEY::F2;
    case GLFW_KEY_F3:
        return KEY::F3;
    case GLFW_KEY_F4:
        return KEY::F4;
    case GLFW_KEY_F5:
        return KEY::F5;
    case GLFW_KEY_F6:
        return KEY::F6;
    case GLFW_KEY_F7:
        return KEY::F7;
    case GLFW_KEY_F8:
        return KEY::F8;
    case GLFW_KEY_F9:
        return KEY::F9;
    case GLFW_KEY_F10:
        return KEY::F10;
    case GLFW_KEY_F11:
        return KEY::F11;
    case GLFW_KEY_F12:
        return KEY::F12;
    case GLFW_KEY_F13:
        return KEY::F13;
    case GLFW_KEY_F14:
        return KEY::F14;
    case GLFW_KEY_F15:
        return KEY::F15;
    case GLFW_KEY_F16:
        return KEY::F16;
    case GLFW_KEY_F17:
        return KEY::F17;
    case GLFW_KEY_F18:
        return KEY::F18;
    case GLFW_KEY_F19:
        return KEY::F19;
    case GLFW_KEY_F20:
        return KEY::F20;
    case GLFW_KEY_F21:
        return KEY::F21;
    case GLFW_KEY_F22:
        return KEY::F22;
    case GLFW_KEY_F23:
        return KEY::F23;
    case GLFW_KEY_F24:
        return KEY::F24;
    case GLFW_KEY_F25:
        return KEY::F25;
    case GLFW_KEY_KP_0:
        return KEY::KP_0;
    case GLFW_KEY_KP_1:
        return KEY::KP_1;
    case GLFW_KEY_KP_2:
        return KEY::KP_2;
    case GLFW_KEY_KP_3:
        return KEY::KP_3;
    case GLFW_KEY_KP_4:
        return KEY::KP_4;
    case GLFW_KEY_KP_5:
        return KEY::KP_5;
    case GLFW_KEY_KP_6:
        return KEY::KP_6;
    case GLFW_KEY_KP_7:
        return KEY::KP_7;
    case GLFW_KEY_KP_8:
        return KEY::KP_8;
    case GLFW_KEY_KP_9:
        return KEY::KP_9;
    case GLFW_KEY_KP_DECIMAL:
        return KEY::KP_DECIMAL;
    case GLFW_KEY_KP_DIVIDE:
        return KEY::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY:
        return KEY::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT:
        return KEY::KP_SUBTRACT;
    case GLFW_KEY_KP_ADD:
        return KEY::KP_ADD;
    case GLFW_KEY_KP_ENTER:
        return KEY::KP_ENTER;
    case GLFW_KEY_KP_EQUAL:
        return KEY::KP_EQUAL;
    case GLFW_KEY_LEFT_SHIFT:
        return KEY::LEFT_SHIFT;
    case GLFW_KEY_LEFT_CONTROL:
        return KEY::LEFT_CONTROL;
    case GLFW_KEY_LEFT_ALT:
        return KEY::LEFT_ALT;
    case GLFW_KEY_LEFT_SUPER:
        return KEY::LEFT_SUPER;
    case GLFW_KEY_RIGHT_SHIFT:
        return KEY::RIGHT_SHIFT;
    case GLFW_KEY_RIGHT_CONTROL:
        return KEY::RIGHT_CONTROL;
    case GLFW_KEY_RIGHT_ALT:
        return KEY::RIGHT_ALT;
    case GLFW_KEY_RIGHT_SUPER:
        return KEY::RIGHT_SUPER;
    case GLFW_KEY_MENU:
        return KEY::MENU;
    }
    return KEY::INVALID;
}

} // namespace notf
