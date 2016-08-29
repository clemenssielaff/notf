#include "core/application.hpp"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>

#include "common/devel.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/key_event.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"

namespace { // anonymous

const size_t WIDGET_RESERVE = 1024; // how many widgets to reserve space for

} // namespace anonymous

namespace signal {

bool register_widget(std::shared_ptr<Widget> widget)
{
    Application& app = Application::get_instance();
    // don't register the Widget if its handle is already been used
    if (app.m_widgets.count(widget->get_handle())) {
        return false;
    }
    app.m_widgets.emplace(std::make_pair(widget->get_handle(), widget));
    return true;
}

Application::Application()
    : m_nextHandle(1024) // 0 is the BAD_HANDLE, the next 1023 handles are reserved for internal use
    , m_log_handler(128, 200) // initial size of the log buffers
    , m_resource_manager()
    , m_render_manager()
    , m_key_states()
    , m_windows()
    , m_widgets()
{
    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, &m_log_handler, std::placeholders::_1));
    m_log_handler.start();

    // set the error callback to catch any errors right away
    glfwSetErrorCallback(Application::on_error);

    m_widgets.reserve(WIDGET_RESERVE);

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
    double last_time = glfwGetTime();
    uint frame_count = 0;

    // loop until there are no more windows open
    while (m_windows.size() > 0) {

        // update all windows
        for (const auto& windowItem : m_windows) {
            windowItem.second->update();
        }

        // poll and process GLWF events
        //glfwWaitEvents();
        glfwPollEvents();

        // print time per frame in ms, averaged over the last second
        ++frame_count;
        if (glfwGetTime() - last_time >= 1.) {
            double ms_per_frame = 1000. / static_cast<double>(frame_count);
            log_debug << ms_per_frame << "ms/frame "
                      << "(" << static_cast<uint>(ms_per_frame / (1. / 6.)) << "%) = "
                      << frame_count << "f/s";
            frame_count = 0;
            last_time += 1.;
        }
    }

    shutdown();
    return to_number(RETURN_CODE::SUCCESS);
}

std::shared_ptr<Widget> Application::get_widget(Handle handle)
{
    auto it = m_widgets.find(handle);
    if (it == m_widgets.end()) {
        log_warning << "Requested Widget with unknown handle:" << handle;
        return {};
    }
    auto widget = it->second.lock();
    if (!widget) {
        m_widgets.erase(it);
        log_warning << "Requested Widget with expired handle:" << handle;
        return {};
    }
    return widget;
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
        log_critical << "Callback for unknown GLFW window";
        return;
    }

    // update the key state
    KEY signal_key = from_glfw_key(key);
    set_key(get_instance().m_key_states, signal_key, action);

    // let the window fire the key event
    KeyEvent key_event{window, signal_key, KEY_ACTION(action), KEY_MODIFIERS(modifiers), get_instance().m_key_states};
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

void Application::shutdown()
{
    static bool is_running = true;
    if (!is_running) {
        return;
    }
    is_running = false;

    log_info << "Application shutdown";
    glfwTerminate();
    m_resource_manager.clear();
    m_log_handler.stop();
    m_log_handler.join();
}

Window* Application::get_window(GLFWwindow* glfw_window)
{
    auto iterator = m_windows.find(glfw_window);
    if (iterator == m_windows.end()) {
        return nullptr;
    }
    return iterator->second;
}

void Application::clean_unused_handles()
{
    std::unordered_map<Handle, std::weak_ptr<Widget>> good_widgets;
    good_widgets.reserve(std::max(good_widgets.size(), WIDGET_RESERVE));
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (!it->second.expired()) {
            good_widgets.emplace(it->first, std::move(it->second));
        }
    }
    m_widgets.swap(good_widgets);
}

} // namespace signal
