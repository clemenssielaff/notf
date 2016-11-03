#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "common/handle.hpp"
#include "common/log.hpp"
#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/layout_root.hpp"
#include "core/object_manager.hpp"
#include "core/render_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/raw_image.hpp"

namespace notf {

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

Size2i Window::get_window_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    int width, height;
    glfwGetWindowSize(m_glfw_window.get(), &width, &height);
    assert(width >= 0);
    assert(height >= 0);
    return {static_cast<uint>(width), static_cast<uint>(height)};
}

Size2i Window::get_framed_window_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    int left, top, right, bottom;
    glfwGetWindowFrameSize(m_glfw_window.get(), &left, &top, &right, &bottom);
    assert(right - left >= 0);
    assert(bottom - top >= 0);
    return {static_cast<uint>(right - left), static_cast<uint>(bottom - top)};
}

Size2i Window::get_canvas_size() const
{
    if (!m_glfw_window) {
        return {};
    }
    int width, height;
    glfwGetFramebufferSize(m_glfw_window.get(), &width, &height);
    assert(width >= 0);
    assert(height >= 0);
    return {static_cast<uint>(width), static_cast<uint>(height)};
}

void Window::_update()
{
    assert(m_glfw_window);

    // make the window current
    Application::get_instance()._set_current_window(shared_from_this());

    // update the viewport buffer
    int width, height;
    glfwGetFramebufferSize(m_glfw_window.get(), &width, &height);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT);

    m_root_widget->_redraw();
    m_render_manager->render(*this);
    glfwSwapBuffers(m_glfw_window.get());
}

void Window::close()
{
    if (m_glfw_window) {
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_root_widget.reset();
        Application::get_instance()._unregister_window(shared_from_this());
        m_glfw_window.reset(nullptr);
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    std::shared_ptr<Window> window = std::make_shared<MakeSmartEnabler<Window>>(info);
    if (!check_gl_error()) {
        log_info << "Created Window '" << window->get_title() << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }

    // inititalize the window
    Application::get_instance()._register_window(window);
    window->m_root_widget = LayoutRoot::create(info.root_widget_handle, window);

    return window;
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

    if (info.opengl_remove_deprecated) {
        if (info.opengl_version_major >= 3) {
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        }
        else {
            log_warning << "Ignored WindowInfo flag 'opengl_remove_deprecated' "
                        << "as it can only be used with OpenGL versions 3.0 or above.";
        }
    }

    if (info.opengl_profile == WindowInfo::PROFILE::ANY) {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    }
    else {
        if ((info.opengl_version_major < 3) || (info.opengl_version_minor < 2)) {
            log_warning << "Ignored WindowInfo field 'opengl_profile' (set to "
                        << (info.opengl_profile == WindowInfo::PROFILE::CORE ? "PROFILE::CORE" : "PROFILE::COMPAT")
                        << ") as it can only be used with OpenGL versions 3.2 or above.";
        }
        else {
            if (info.opengl_profile == WindowInfo::PROFILE::CORE) {
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            }
            else {
                assert(info.opengl_profile == WindowInfo::PROFILE::COMPAT);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
            }
        }
    }

    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    glfwWindowHint(GLFW_SAMPLES, std::max(0, info.samples));

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        log_fatal << "Window or context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::GLFW_FAILURE));
    }

    // setup OpenGl
    glfwMakeContextCurrent(m_glfw_window.get());
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSwapInterval(info.enable_vsync ? 1 : 0);
    glClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);

    // apply the Window icon
    // In order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!info.icon.empty()) {
        const std::string icon_path = app.get_resource_manager().get_texture_directory() + info.icon;
        try {
            Image icon(icon_path);
            if (icon.get_bytes_per_pixel() != 4) {
                log_warning << "Icon file '" << icon_path
                            << "' does not provide the required 4 byte per pixel, but " << icon.get_bytes_per_pixel();
            }
            else {
                const GLFWimage glfw_icon{icon.get_width(), icon.get_height(), const_cast<uchar*>(icon.get_data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
            }
        }
        catch (std::runtime_error) {
            log_warning << "Failed to load Window icon '" << icon_path << "'";
        }
    }
}

void Window::_on_resize(int width, int height)
{
    m_root_widget->relayout({static_cast<Real>(width), static_cast<Real>(height)});
}

} // namespace notf
