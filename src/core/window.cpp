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
    , m_root_widget(Widget::make_root_widget(this))
    , m_render_manager()
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
    glfwSwapInterval(info.enable_vsync ? 1 : 0);

    // TODO: window info variable for clear color?
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glViewport(0, 0, info.width, info.height);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // log error or success
    if(!check_gl_error()){
        log_info << "Created Window '" << m_title << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }
}

Window::~Window()
{
    close();
}

Size2 Window::get_window_size() const
{
    int width, height;
    glfwGetWindowSize(m_glfw_window.get(), &width, &height);
    assert(width >= 0);
    assert(height >= 0);
    return {static_cast<uint>(width), static_cast<uint>(height)};
}

Size2 Window::get_framed_window_size() const
{
    int left, top, right, bottom;
    glfwGetWindowFrameSize(m_glfw_window.get(), &left, &top, &right, &bottom);
    assert(right - left >= 0);
    assert(bottom - top >= 0);
    return {static_cast<uint>(right - left), static_cast<uint>(bottom - top)};
}

Size2 Window::get_canvas_size() const
{
    int width, height;
    glfwGetFramebufferSize(m_glfw_window.get(), &width, &height);
    assert(width >= 0);
    assert(height >= 0);
    return {static_cast<uint>(width), static_cast<uint>(height)};
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
    // make the window current
    Application::get_instance().set_current_window(this);

    // update the viewport buffer
    int width, height;
    glfwGetFramebufferSize(m_glfw_window.get(), &width, &height);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT);

    m_root_widget->redraw();
    m_render_manager.render(*this);
    glfwSwapBuffers(m_glfw_window.get());
}

} // namespace signal
