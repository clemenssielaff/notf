#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "common/handle.hpp"
#include "common/log.hpp"
#include "core/application.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/item_manager.hpp"
#include "core/key_event.hpp"
#include "core/render_manager.hpp"
#include "core/old_layout_root.hpp"
#include "core/widget.hpp"
#include "graphics/gl_errors.hpp"

namespace signal {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

Window::~Window()
{
    close();
    // TODO: OpenGL Components and Textures etc. can survive the destruction of a Window and its OpenGL Context!
    // This seems to cause a crash on shutdown when running in OpenGL 4.5 but strangely not in 3.0.
    // I guess because the error is just silently ignored?
    // Anyway, there should either be a single, shared Context per Application, so that I can just destroy all
    // Components before the Application's Context is closed, or ... Context dependent Components? (nah.)
}

Size2i Window::get_window_size() const
{
    int width, height;
    glfwGetWindowSize(m_glfw_window.get(), &width, &height);
    assert(width >= 0);
    assert(height >= 0);
    return {static_cast<uint>(width), static_cast<uint>(height)};
}

Size2i Window::get_framed_window_size() const
{
    int left, top, right, bottom;
    glfwGetWindowFrameSize(m_glfw_window.get(), &left, &top, &right, &bottom);
    assert(right - left >= 0);
    assert(bottom - top >= 0);
    return {static_cast<uint>(right - left), static_cast<uint>(bottom - top)};
}

Size2i Window::get_canvas_size() const
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
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_root_widget.reset();
        Application::get_instance().unregister_window(this);
        m_glfw_window.reset(nullptr);
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    std::shared_ptr<Window> window = std::make_shared<MakeSmartEnabler<Window>>(info);
    window->m_root_widget = OldLayoutRoot::create(info.root_widget_handle, window);
    log_trace << "Assigned RootLayoutItem with handle: " << window->m_root_widget->get_handle()
              << " to Window \"" << window->get_title() << "\"";
    return window;
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
    m_render_manager->render(*this);
    glfwSwapBuffers(m_glfw_window.get());
}

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr, window_deleter)
    , m_title(info.title)
    , m_root_widget()
    , m_render_manager(std::make_unique<RenderManager>())
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
    glClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);

    // log error or success
    if (!check_gl_error()) {
        log_info << "Created Window '" << m_title << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }
}

} // namespace signal
