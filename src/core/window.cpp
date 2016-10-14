#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "common/handle.hpp"
#include "common/log.hpp"
#include "core/application.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/key_event.hpp"
#include "core/layout_item_manager.hpp"
#include "core/render_manager.hpp"
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
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_root_widget.reset();
        Application::get_instance().unregister_window(this);
        m_glfw_window.reset(nullptr);
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    return std::make_shared<MakeSmartEnabler<Window>>(info);
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
    , m_root_widget(WindowWidget::create(info.root_widget_handle, shared_from_this()))
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
    glClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);

    // log error or success
    if (!check_gl_error()) {
        log_info << "Created Window '" << m_title << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }
}

WindowWidget::WindowWidget(Handle handle, std::weak_ptr<Window> window)
    : Widget(handle)
    , m_window(std::move(window))
{
}

std::shared_ptr<WindowWidget> WindowWidget::create(Handle handle, std::weak_ptr<Window> window)
{
    // make sure there's a valid handle for the RootWidget
    LayoutItemManager& manager = Application::get_instance().get_layout_item_manager();
    if (!handle) {
        handle = manager.get_next_handle();
    }

    // create the RootWidget and try to register it with the Application
    std::shared_ptr<WindowWidget> root_widget = std::make_shared<MakeSmartEnabler<WindowWidget>>(handle, std::move(window));
    if (!manager.register_item(root_widget)) {
        log_critical << "Cannot register Window RootWidget with handle " << handle
                     << " because the handle is already taken";
        return {};
    }

    // finalize the RootWidget's creation
    log_trace << "Created RootWidget with handle: " << handle << " for Window \"" << window.lock()->get_title() << "\"";
    return root_widget;
}

} // namespace signal
