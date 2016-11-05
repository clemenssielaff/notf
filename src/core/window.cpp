#include "core/window.hpp"

#include <iostream>
#include <utility>

#include "core/glfw_wrapper.hpp"

#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg/nanovg.h>
#include <nanovg/nanovg_gl.h>

#include "common/handle.hpp"
#include "common/log.hpp"
#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/layout_root.hpp"
#include "core/object_manager.hpp"
#include "core/render_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/raw_image.hpp"
#include "graphics/mytest.hpp"

namespace notf {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

void nanovg_deleter(NVGcontext* nvg_context)
{
    if(nvg_context){
        nvgDeleteGLES3(nvg_context);
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

    // query window properties
    int window_width, window_height;
    glfwGetWindowSize(m_glfw_window.get(), &window_width, &window_height);

    int buffer_width, buffer_height;
    glfwGetFramebufferSize(m_glfw_window.get(), &buffer_width, &buffer_height);

    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);

    const float pixel_ratio = window_width ? static_cast<float>(buffer_width) / static_cast<float>(window_width) : 0;

    glViewport(0, 0, buffer_width, buffer_height);
    // set clear color here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    nvgBeginFrame(m_nvg_context.get(), window_width, window_height, pixel_ratio);
    //        renderDemo(vg, static_cast<float>(mx), static_cast<float>(my),
    //                   winWidth, winHeight, static_cast<float>(glfwGetTime()), 0, &data);
    doit(m_nvg_context.get());
    nvgEndFrame(m_nvg_context.get());

    glEnable(GL_DEPTH_TEST);

//    m_root_widget->_redraw();
//    m_render_manager->render(*this);
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
    , m_nvg_context(nullptr, nanovg_deleter)
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

    // NoTF uses OpenGL ES 3.0 for now
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        log_fatal << "Window or OpenGL context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::GLFW_FAILURE));
    }

    // create the NanoVG context
    glfwMakeContextCurrent(m_glfw_window.get());
    m_nvg_context.reset(nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES
#ifdef _DEBUG
                                       | NVG_DEBUG
#endif
                                       ));
    if (!m_nvg_context) {
        log_fatal << "NanoVG context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::NANOVG_FAILURE));
    }

    // initial OpenGl setup
    glfwSwapInterval(info.enable_vsync ? 1 : 0);
    glClearColor(info.clear_color.r, info.clear_color.g, info.clear_color.b, info.clear_color.a);
    // TODO: setting the clear color here will only apply until the next wndow is opened
    // Instead, there should be a "background color" field of the Window - leading back to the question of a color class

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
