#include "core/window.hpp"

#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/application.hpp"
#include "core/events/key_event.hpp"
#include "core/events/mouse_event.hpp"
#include "core/glfw.hpp"
#include "core/render_manager.hpp"
#include "core/resource_manager.hpp"
#include "core/widget.hpp"
#include "core/window_layout.hpp"
#include "graphics/cell/cell_canvas.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/raw_image.hpp"

namespace notf {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

struct Window::make_shared_enabler : public Window {
    template <typename... Args>
    make_shared_enabler(Args&&... args)
        : Window(std::forward<Args>(args)...) {}
    virtual ~make_shared_enabler();
};
Window::make_shared_enabler::~make_shared_enabler() {}

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr, window_deleter)
    , m_title(info.title)
    , m_layout()
    , m_render_manager() // has to be created after the OpenGL Context
    , m_graphics_context()
    , m_cell_context()
    , m_background_color(info.clear_color)
    , m_size(info.size)
{
    // make sure that an Application was initialized before instanciating a Window (will exit on failure)
    Application& app = Application::get_instance();

    // close when the user presses ESC
    connect_signal(on_token_key, &Window::close, [](const KeyEvent& event) { return event.key == Key::ESCAPE; });

    // NoTF uses OpenGL ES 3.0 for now
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, info.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(m_size.width, m_size.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        log_fatal << "Window or OpenGL context creation failed for Window '" << get_title() << "'";
        app._shutdown();
        exit(to_number(Application::RETURN_CODE::GLFW_FAILURE));
    }
    glfwSetWindowUserPointer(m_glfw_window.get(), this);
    glfwMakeContextCurrent(m_glfw_window.get());
    glfwSwapInterval(app.get_info().enable_vsync ? 1 : 0);

    // create the auxiliary objects
    m_render_manager = std::make_unique<RenderManager>(this);

    GraphicsContextOptions context_args;
    context_args.pixel_ratio = static_cast<float>(get_buffer_size().width) / static_cast<float>(get_window_size().width);
    m_graphics_context       = std::make_unique<GraphicsContext>(this, context_args);

    m_cell_context = std::make_unique<CellCanvas>(*m_graphics_context);

    // apply the Window icon
    // In order to show the icon in Ubuntu 16.04 is as bit more complicated:
    // 1. start the software
    // 2. copy the icon to ~/.local/share/icons/hicolor/<resolution>/apps/<yourapp>
    // 3. modify the file ~/.local/share/applications/<yourapp>, in particular the line Icon=<yourapp>
    //
    // in order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!info.icon.empty()) {
        const std::string icon_path = app.get_resource_manager().get_texture_directory() + info.icon;
        try {
            RawImage icon(icon_path);
            if (icon.get_bytes_per_pixel() != 4) {
                log_warning << "Icon file '" << icon_path
                            << "' does not provide the required 4 byte per pixel, but " << icon.get_bytes_per_pixel();
            }
            else {
                const GLFWimage glfw_icon{icon.get_width(), icon.get_height(), const_cast<uchar*>(icon.get_data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
            }
        } catch (std::runtime_error) {
            log_warning << "Failed to load Window icon '" << icon_path << "'";
        }
    }
}

std::shared_ptr<Window> Window::create(const WindowInfo& info)
{
    std::shared_ptr<Window> window = std::make_shared<make_shared_enabler>(info);
    if (get_gl_error()) {
        exit(to_number(Application::RETURN_CODE::OPENGL_FAILURE));
    }
    else {
        log_info << "Created Window '" << window->get_title() << "' "
                 << "using OpenGl version: " << glGetString(GL_VERSION);
    }

    // inititalize the window
    Application::get_instance()._register_window(window);
    window->m_layout = WindowLayout::create(window);
    window->m_layout->_set_size(window->get_buffer_size());

    return window;
}

Window::~Window()
{
    close();
}

Size2i Window::get_framed_window_size() const
{
    if (!m_glfw_window) {
        return Size2i::invalid();
    }
    int left, top, right, bottom;
    glfwGetWindowFrameSize(m_glfw_window.get(), &left, &top, &right, &bottom);
    Size2i result = {right - left, bottom - top};
    assert(result.is_valid());
    return result;
}

Size2i Window::get_buffer_size() const
{
    if (!m_glfw_window) {
        return Size2i::invalid();
    }
    Size2i result;
    glfwGetFramebufferSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

Vector2f Window::get_mouse_pos() const
{
    if (!m_glfw_window) {
        return Vector2f::zero();
    }
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);
    return {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};
}

void Window::_update()
{
    assert(m_glfw_window);

    // do nothing, if there are no Widgets in need to be redrawn
    //    if (m_render_manager->is_clean()) { // TODO: dirty mechanism for RenderManager
    //        return;
    //    }

    // make the window current
    Application::get_instance()._set_current_window(this);

    // prepare the viewport
    Size2i buffer_size;
    glfwGetFramebufferSize(m_glfw_window.get(), &buffer_size.width, &buffer_size.height);

    glViewport(0, 0, buffer_size.width, buffer_size.height);
    glClearColor(m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // render
    try {
        m_render_manager->render(buffer_size);
    }
    // if an error bubbled all the way up here, something has gone horribly wrong
    catch (std::runtime_error error) {
        log_critical << "Rendering failed: \"" << error.what() << "\"";
    }

    // post-render
    glfwSwapBuffers(m_glfw_window.get());
}

void Window::close()
{
    if (m_glfw_window) {
        log_trace << "Closing Window \"" << m_title << "\"";
        on_close(*this);
        m_layout.reset();
        Application::get_instance()._unregister_window(shared_from_this());
        m_glfw_window.reset(nullptr);
    }
    m_size = Size2i::invalid();
}

void Window::_on_resize(int width, int height)
{
    m_size.width  = width;
    m_size.height = height;
    m_layout->_set_size(get_buffer_size());
}

void Window::_propagate_mouse_event(MouseEvent&& event)
{
    // TODO: I suspect mouse event propagation is suboptimal - also, it doesn't propagate up the hierarchy!

    // sort Widgets by render RenderLayers
    std::vector<Widget*> widgets;
    {
        std::vector<std::vector<Widget*>> widgets_by_layer(m_render_manager->get_layer_count());
        for (Widget* widget : m_layout->get_widgets_at(event.window_pos)) {
            RenderLayer* render_layer = widget->get_render_layer().get();
            assert(render_layer);
            widgets_by_layer[render_layer->get_index()].emplace_back(widget);
        }
        widgets = flatten(widgets_by_layer);
    }

// TODO: CONTINUE HERE


    // call the appropriate event signal
    if (event.action == MouseAction::MOVE) {
        for (const auto& layer : widgets_by_layer) {
            for (const auto& widget : layer) {
                widget->on_mouse_move(event);
                if (event.was_handled()) {
                    return;
                }
            }
        }
    }
    else if (event.action == MouseAction::PRESS
             || event.action == MouseAction::RELEASE) {
        for (const auto& layer : widgets_by_layer) {
            for (const auto& widget : layer) {
                widget->on_mouse_button(event);
                if (event.was_handled()) {
                    return;
                }
            }
        }
    }
    else if (event.action == MouseAction::SCROLL) {
        for (const auto& layer : widgets_by_layer) {
            for (const auto& widget : layer) {
                widget->on_scroll(event);
                if (event.was_handled()) {
                    return;
                }
            }
        }
    }
    else {
        assert(0);
    }
}

} // namespace notf
