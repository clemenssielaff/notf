#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/keyboard.hpp"
#include "core/widget.hpp"

namespace signal {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr, window_deleter)
    , m_title(info.title)
    , m_root_widget(Widget::make_widget())
{
    // always make sure that the Application is constructed first
    Application& app = Application::get_instance();

    // close when the user presses ESC
    connect(on_token_key,
        [this](const KeyEvent&) { close(); },
        [](const KeyEvent& event) { return event.key == KEY::ESCAPE; });

    // set context variables before creating the window
    if (info.opengl_version_major >= 0) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, info.opengl_version_major);
    }
    if (info.opengl_version_minor >= 0) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, info.opengl_version_minor);
    }
    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr));

    // register with the application (if the GLFW window creation failed, this call will exit the application)
    app.register_window(this);

    // setup OpenGl
    glfwMakeContextCurrent(m_glfw_window.get());
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    log_debug << "Created Window '" << m_title << "' "
              << "using OpenGl version: " << glad_glGetString(GL_VERSION);
}

Window::~Window()
{
    close();
}

void Window::close()
{
    if (m_glfw_window) {
        on_close(*this);
        log_debug << "Closing Window \"" << m_title << "\"";
        Application::get_instance().unregister_window(this);
        m_glfw_window.reset(nullptr);
    }
}

void Window::update()
{
    glClear(GL_COLOR_BUFFER_BIT);

    m_root_widget->redraw();

    glfwSwapBuffers(m_glfw_window.get());
}

} // namespace signal
