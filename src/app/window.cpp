#include "app/window.hpp"

#include "app/event_manager.hpp"
#include "app/glfw.hpp"
#include "app/layer.hpp"
#include "app/property_graph.hpp"
#include "app/render_manager.hpp"
#include "app/resource_manager.hpp"
#include "app/scene.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/raw_image.hpp"
#include "graphics/text/font_manager.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

void window_deleter(GLFWwindow* glfw_window)
{
    if (glfw_window) {
        glfwDestroyWindow(glfw_window);
    }
}

} // namespace detail

//====================================================================================================================//

window_initialization_error::~window_initialization_error() {}

//====================================================================================================================//

const Window::Args Window::s_default_args = {};

Window::Window(const Args& args)
    : m_glfw_window(nullptr, detail::window_deleter), m_title(args.title), m_state(args.state), m_size(args.size)
{
    // make sure that an Application was initialized before instanciating a Window (will throw on failure)
    Application& app = Application::instance();

    // NoTF uses OpenGL ES 3.2
    //    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, args.is_resizeable ? GL_TRUE : GL_FALSE);

    // create the GLFW window
    m_glfw_window.reset(glfwCreateWindow(m_size.width, m_size.height, m_title.c_str(), nullptr, nullptr));
    if (!m_glfw_window) {
        notf_throw_format(window_initialization_error, "Window or OpenGL context creation failed for Window \"{}\"",
                          title());
    }
    glfwSetWindowUserPointer(m_glfw_window.get(), this);
    set_state(m_state);

    // create the auxiliary objects
    m_graphics_context = std::make_unique<GraphicsContext>(m_glfw_window.get());
    {
        auto guard = m_graphics_context->make_current();
        m_property_graph = std::make_unique<PropertyGraph>();
        m_scene_graph = std::make_unique<SceneGraph>(*this);
        m_font_manager = FontManager::create(*m_graphics_context);
    }

    // connect the window callbacks
    EventManager::Access<Window>().register_window(*this);

    // apply the Window icon
    // Showing the icon in Ubuntu 16.04 is as bit more complicated:
    // 1. start the software at least once, you might have to pin it to the launcher as well ...
    // 2. copy the icon to ~/.local/share/icons/hicolor/<resolution>/apps/<yourapp>
    // 3. modify the file ~/.local/share/applications/<yourapp>, in particular the line Icon=<yourapp>
    //
    // in order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!args.icon.empty()) {
        const std::string icon_path = app.resource_manager().texture_directory() + args.icon;
        try {
            RawImage icon(icon_path);
            if (icon.channels() != 4) {
                log_warning << "Icon file \"" << icon_path << "\" does not provide the required 4 byte per pixel, but "
                            << icon.channels();
            }
            else {
                const GLFWimage glfw_icon{icon.width(), icon.height(), const_cast<uchar*>(icon.data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
                log_trace << "Loaded icon for Window \"" << title() << "\" from \"" << icon_path << "\"";
            }
        }
        catch (std::runtime_error) {
            log_warning << "Failed to load icon for Window \"" << title() << "\" from \"" << icon_path << "\"";
        }
    }

    log_info << "Created Window \"" << title() << "\" using OpenGl version: " << glGetString(GL_VERSION);
}

WindowPtr Window::_create(const Args& args)
{
    // inititalize the window
    WindowPtr window = NOTF_MAKE_UNIQUE_FROM_PRIVATE(Window, args);
    return window;
}

Window::~Window() { close(); }

Size2i Window::framed_window_size() const
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

Size2i Window::buffer_size() const
{
    if (!m_glfw_window) {
        return Size2i::invalid();
    }
    Size2i result;
    glfwGetFramebufferSize(m_glfw_window.get(), &result.width, &result.height);
    assert(result.is_valid());
    return result;
}

Vector2f Window::mouse_pos() const
{
    if (!m_glfw_window) {
        return Vector2f::zero();
    }
    double mouse_x, mouse_y;
    glfwGetCursorPos(m_glfw_window.get(), &mouse_x, &mouse_y);
    return {static_cast<float>(mouse_x), static_cast<float>(mouse_y)};
}

void Window::request_redraw() { Application::instance().render_manager().render(this); }

void Window::set_state(const State state)
{
    switch (state) {
    case State::MINIMIZED:
        glfwIconifyWindow(m_glfw_window.get());
        break;
    case State::WINDOWED:
        glfwRestoreWindow(m_glfw_window.get());
        break;
    case State::FULLSCREEN:
        glfwMaximizeWindow(m_glfw_window.get());
        break;
    }
}

void Window::close()
{
    if (!m_glfw_window) {
        return;
    }

    log_trace << "Closing Window \"" << m_title << "\"";

    // disconnect the window callbacks
    EventManager::Access<Window>().remove_window(*this);

    m_scene_graph.reset();
    m_property_graph.reset();
    m_font_manager.reset();
    m_graphics_context.reset();
    m_glfw_window.reset();

    m_size = Size2i::invalid();

    Application::Access<Window>().unregister(this);
}

NOTF_CLOSE_NAMESPACE
