#include "app/application.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>

#include "app/window.hpp"
#include "common/devel.hpp"
#include "common/keys.hpp"

namespace signal {

Application::Application()
    : m_windows()
    , m_log_handler(128, 200) // initial size of the log buffers
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, &m_log_handler, std::placeholders::_1));
    m_log_handler.start();

    // set the error callback to catch any errors right away
    glfwSetErrorCallback(Application::on_error);

    // initialize GLFW
    if (!glfwInit()) {
        log_fatal << "GLFW initialization failed";
        shutdown();
        exit(to_number(RETURN_CODE::FAILURE));
    }
}

Application::~Application()
{
    shutdown();
}

int Application::exec()
{
    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (const auto& windowItem : m_windows) {
            windowItem.second->update();
        }

        // poll and process GLWF events
        glfwPollEvents();
    }

    shutdown();
    return to_number(RETURN_CODE::SUCCESS);
}

void Application::on_error(int error, const char* message)
{
    std::cerr << "GLFW Error " << error << ": '" << message << "'" << std::endl;
}

void Application::on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int mods)
{
    UNUSED(scancode);
    Window* window = instance().get_window(glfw_window);
    if (!window) {
        log_critical << "Callback to unknown GLFW window";
        return;
    }
    window->on_token_key(static_cast<KEY>(key), static_cast<KEY_ACTION>(action), static_cast<KEY_MODS>(mods));
}

void Application::on_window_close(GLFWwindow* glfw_window)
{
    Window* window = instance().get_window(glfw_window);
    if (!window) {
        log_critical << "Callback to unknown GLFW window";
        return;
    }
    window->close();
}

void Application::register_window(Window* window)
{
    GLFWwindow* glfw_window = window->get_glwf_window();
    if (!glfw_window) {
        log_fatal << "Window or context creation failed for window '" << window->title() << "'";
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
    GLFWwindow* glfw_window = window->get_glwf_window();
    auto iterator = m_windows.find(glfw_window);
    assert(iterator != m_windows.end());

    // disconnect the window callbacks
    glfwSetWindowCloseCallback(glfw_window, nullptr);
    glfwSetKeyCallback(glfw_window, nullptr);

    // unregister the window
    log_debug << "Unregistered window: " << window->title();
    m_windows.erase(iterator);
}

void Application::shutdown()
{
    static bool is_running = true;
    if(is_running){
        log_info << "Application shutdown";
        m_log_handler.stop();
        glfwTerminate();
        m_log_handler.join();
        is_running = false;
    }
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
