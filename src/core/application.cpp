#include "core/application.hpp"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>

#include "common/log.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/key_event.hpp"
#include "core/keyboard.hpp"
#include "core/layout_object.hpp"
#include "core/item_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/window.hpp"
#include "utils/unused.hpp"

namespace { // anonymous

/// \brief The current state of all keyboard keys.
/// Is unit-global instead of a member variable to keep the application header small.
signal::KeyStateSet g_key_states;

} // namespace anonymous

namespace signal {

Application::Application()
    : m_log_handler(std::make_unique<LogHandler>(128, 200)) // initial size of the log buffers
    , m_resource_manager(std::make_unique<ResourceManager>())
    , m_layout_item_manager(std::make_unique<ItemManager>(1024)) // reserve space for 1024 LayoutItems right away
    , m_windows()
    , m_current_window(nullptr)
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, m_log_handler.get(), std::placeholders::_1));
    m_log_handler->start();

    // set the error callback to catch any errors right away
    glfwSetErrorCallback(Application::on_error);

    // initialize GLFW
    if (!glfwInit()) {
        log_fatal << "GLFW initialization failed";
        shutdown();
        exit(to_number(RETURN_CODE::FAILURE));
    }
    log_info << "Started application";
    log_info << "GLFW version: " << glfwGetVersionString();
}

Application::~Application()
{
    shutdown();
}

int Application::exec()
{
    //    double last_time = glfwGetTime();
    //    uint frame_count = 0;

    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (const auto& windowItem : m_windows) {
            windowItem.second->update();
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

    shutdown();
    return to_number(RETURN_CODE::SUCCESS);
}

void Application::on_error(int error, const char* message)
{
    std::cerr << "GLFW Error " << error << ": '" << message << "'" << std::endl;
}

void Application::on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers)
{
    UNUSED(scancode);
    Window* window = get_instance().get_window(glfw_window);
    if (!window) {
        log_critical << "Received 'on_token_key' Callback for unknown GLFW window";
        return;
    }

    // update the key state
    KEY signal_key = from_glfw_key(key);
    set_key(g_key_states, signal_key, action);

    // let the window fire the key event
    KeyEvent key_event{window, signal_key, KEY_ACTION(action), KEY_MODIFIERS(modifiers), g_key_states};
    window->on_token_key(key_event);
}

void Application::on_window_close(GLFWwindow* glfw_window)
{
    Window* window = get_instance().get_window(glfw_window);
    if (!window) {
        log_critical << "Callback for unknown GLFW window";
        return;
    }
    window->close();
}

void Application::register_window(Window* window)
{
    GLFWwindow* glfw_window = window->glwf_window();
    if (!glfw_window) {
        log_fatal << "Window or context creation failed for window '" << window->get_title() << "'";
        shutdown();
        exit(to_number(RETURN_CODE::FAILURE));
    }
    assert(m_windows.count(glfw_window) == 0);

    // register the window
    m_windows.emplace(glfw_window, window);

    // connect the window callbacks
    glfwSetWindowCloseCallback(glfw_window, on_window_close);
    glfwSetKeyCallback(glfw_window, on_token_key);
}

void Application::unregister_window(Window* window)
{
    assert(window);
    GLFWwindow* glfw_window = window->glwf_window();
    auto iterator = m_windows.find(glfw_window);
    assert(iterator != m_windows.end());

    // disconnect the window callbacks
    glfwSetWindowCloseCallback(glfw_window, nullptr);
    glfwSetKeyCallback(glfw_window, nullptr);

    // unregister the window
    m_windows.erase(iterator);
}

void Application::set_current_window(Window* window)
{
    if (m_current_window != window) {
        glfwMakeContextCurrent(window->glwf_window());
        m_current_window = window;
    }
}

void Application::shutdown()
{
    static bool is_running = true;
    if (!is_running) {
        return;
    }
    is_running = false;

    // stop the event loop
    glfwTerminate();

    // close all remaining windows
    for (auto it : m_windows) {
        it.second->close();
    }

    // release all resources
    m_resource_manager->clear();

    // stop the logger
    log_info << "Application shutdown";
    m_log_handler->stop();
    m_log_handler->join();
}

Window* Application::get_window(GLFWwindow* glfw_window)
{
    auto iterator = m_windows.find(glfw_window);
    if (iterator == m_windows.end()) {
        return nullptr;
    }
    return iterator->second;
}

} // namespace signal
