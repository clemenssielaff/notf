#include "app/application.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>

#include "app/window.hpp"
#include "common/devel.hpp"

namespace untitled {

Application::Application()
    : m_errors()
    , m_windows()
    , m_log_handler(128, 200) // initial size of the log buffers
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, &m_log_handler, std::placeholders::_1));
    m_log_handler.start();

    // set the error callback to catch any errors right away
    glfwSetErrorCallback(Application::on_error);

    // initialize GLFW
    if (!glfwInit()) {
        on_error(GLFW_INITIALIZATION_FAILURE, "GLFW initialization failed");
    }
}

Application::~Application()
{
    log_info << "Application shutdown";
    m_log_handler.stop();
    glfwTerminate();
    m_log_handler.join();
}

Window* Application::create_window(const WindowInfo& info)
{
    // create the new Window
    Window* window = new Window(info);

    // delete the Window again, if something has gone wrong
    GLFWwindow* glfw_window = window->get_glwf_window();
    if (!glfw_window) {
        std::stringstream ss;
        ss << "Window or context creation failed for window '" << window->get_title() << "'";
        on_error(WINDOW_CONTEXT_CREATION_FAILURE, ss.str().c_str());

        delete window;
        return nullptr;
    }

    // store the new Window
    m_windows.push_back({ glfw_window, window });
    assert(m_windows.size() == 1); // only one window is supported right now

    glfwMakeContextCurrent(glfw_window); // TODO: figure out how to support multi-window applications
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); // TODO: this is most certainly not the right plcae to put this

    // connect its callbacks
    glfwSetKeyCallback(glfw_window, on_token_key);

    return window;
}

Window* Application::create_window()
{
    WindowInfo defaultInfo;
    return create_window(defaultInfo);
}

int Application::exec()
{
    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (pair<GLFWwindow*, Window*>& windowItem : m_windows) {
            windowItem.second->update();
        }

        // poll and process GLWF events
        glfwPollEvents();

        // destroy closed windows
        for (size_t index = m_windows.size(); index > 0; --index) {
            if (m_windows[index - 1].second->is_closing()) {
                close_window(index - 1);
            }
        }
    }

    return to_number(RETURN_CODE::SUCCESS);
}

vector<Error> Application::get_errors()
{
    vector<Error> result;
    result.swap(m_errors);
    assert(m_errors.empty());
    return result;
}

Window* Application::get_window(const GLFWwindow* glfw_window)
{
    for (pair<GLFWwindow*, Window*>& item : m_windows) {
        if (glfw_window == item.first) {
            return item.second;
        }
    }
    return nullptr;
}

void Application::close_window(size_t index)
{
    assert(index < m_windows.size());
    GLFWwindow* glfw_window = m_windows[index].first;
    Window* window = m_windows[index].second;

    // remove the window from the application
    m_windows[index] = m_windows.back();
    m_windows.pop_back();

    // disconnect the callbacks
    glfwSetKeyCallback(glfw_window, nullptr);

    // close the window
    delete window;
}

void Application::on_error(int error, const char* message)
{
    get_instance().m_errors.push_back({ error, message });
    std::cerr << "Error " << error << ": '" << message << "'" << std::endl; // TOOD: proper logging with context info
}

void Application::on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int mods)
{
    UNUSED(scancode);
    Window* window = get_instance().get_window(glfw_window);
    if (!window) {
        on_error(CALLBACK_FOR_UNKNOWN_GLFW_WINDOW, "Unknown GLFW window in 'on_token_key'");
        return;
    }
    window->on_token_key(static_cast<KEY>(key), static_cast<KEY_ACTION>(action), static_cast<KEY_MODS>(mods));
}

} // namespace untitled
