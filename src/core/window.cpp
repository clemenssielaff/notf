#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "common/log.hpp"
#include "core/application.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/key_event.hpp"
#include "core/widget.hpp"
#include "graphics/gl_errors.hpp"

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
    glfwWindowHint(GLFW_SAMPLES, std::max(0, info.samples));

    // create the GLFW window (error test in next step)
    m_glfw_window.reset(glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr));

    // register with the application (if the GLFW window creation failed, this call will exit the application)
    app.register_window(this);

    // setup OpenGl
    glfwMakeContextCurrent(m_glfw_window.get());
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    // TODO: window info variable for clear color?
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // log error or success
    GLenum gl_error = glad_glGetError();
    if (gl_error != GL_NO_ERROR) {
        log_critical << "OpenGL error:" << gl_error_string(gl_error);
    } else {
        log_info << "Created Window '" << m_title << "' "
                 << "using OpenGl version: " << glad_glGetString(GL_VERSION);
    }
}

Window::~Window()
{
    close();
}

void Window::close()
{
    if (m_glfw_window) {
        log_debug << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_root_widget.reset();
        Application::get_instance().unregister_window(this);
        m_glfw_window.reset(nullptr);
    }
}

void Window::update()
{
    // TODO: is it really necessary to set the context every time?
    if(glfwGetCurrentContext() != m_glfw_window.get()){ // and how much does it really cost just setting it twice?
        glfwMakeContextCurrent(m_glfw_window.get());
    }

    int width, height;
    glfwGetFramebufferSize(m_glfw_window.get(), &width, &height);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT);

    m_root_widget->redraw();

    glfwSwapBuffers(m_glfw_window.get());
}

} // namespace signal
