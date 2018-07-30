#include "app/window.hpp"

#include "app/event_manager.hpp"
#include "app/glfw.hpp"
#include "app/render_manager.hpp"
#include "app/scene.hpp"
#include "common/log.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/raw_image.hpp"

namespace {

GLFWmonitor* get_glfw_monitor(int index)
{
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    NOTF_ASSERT(index < count);
    return monitors[index];
}

} // namespace

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

Window::initialization_error::~initialization_error() = default;

// ================================================================================================================== //

const Window::Settings Window::s_default_settings = {};

Window::Window(const Settings& settings)
    : m_glfw_window(nullptr, detail::window_deleter)
    , m_settings(std::make_unique<Settings>(_validate_settings(settings)))
{
    // make sure that an Application was initialized before instanciating a Window (will throw on failure)
    TheApplication& app = TheApplication::get();

    // Window and OpenGL context hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_VISIBLE, settings.is_visible);
    glfwWindowHint(GLFW_FOCUSED, settings.is_focused);
    glfwWindowHint(GLFW_DECORATED, settings.is_decorated);
    glfwWindowHint(GLFW_RESIZABLE, settings.is_resizeable);
    glfwWindowHint(GLFW_MAXIMIZED, settings.state == Settings::State::MAXIMIZED);
    glfwWindowHint(GLFW_SAMPLES, settings.samples);
    glfwWindowHint(GLFW_DOUBLEBUFFER, true);
    glfwWindowHint(GLFW_SRGB_CAPABLE, true);
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
    glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_NONE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, is_debug_build());
    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, !is_debug_build());

    { // create the GLFW window with the correct size and monitor
        Size2i window_size = m_settings->size;
        GLFWmonitor* monitor = nullptr;
        if (m_settings->state == Window::Settings::State::FULLSCREEN) {
            if (m_settings->has_buffer_size()) {
                window_size = m_settings->buffer_size;
            }
            else {
                monitor = get_glfw_monitor(m_settings->has_monitor() ? m_settings->monitor : 0);
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                window_size = {mode->width, mode->height};
            }
        }

        m_glfw_window.reset(
            glfwCreateWindow(window_size.width, window_size.height, get_title().c_str(), monitor, nullptr));
        if (!m_glfw_window) {
            NOTF_THROW(initialization_error, "Window or OpenGL context creation failed for Window \"{}\"", get_title());
        }
        glfwSetWindowUserPointer(m_glfw_window.get(), this);

        // position the window
        if (m_settings->state == Settings::State::MINIMIZED || m_settings->state == Settings::State::MAXIMIZED) {
            set_state(m_settings->state);
        }
    }

    // create the graphics context
    m_graphics_context = std::make_unique<GraphicsContext>(m_glfw_window.get());
    auto context_guard = m_graphics_context->make_current();

    // connect the window callbacks
    EventManager::Access<Window>::register_window(TheApplication::get().get_event_manager(), *this);

    // apply the Window icon
    // Showing the icon in Ubuntu 16.04 is as bit more complicated:
    // 1. start the software at least once, you might have to pin it to the launcher as well ...
    // 2. copy the icon to ~/.local/share/icons/hicolor/<resolution>/apps/<yourapp>
    // 3. modify the file ~/.local/share/applications/<yourapp>, in particular the line Icon=<yourapp>
    //
    // in order to remove leftover icons on Ubuntu call:
    // rm ~/.local/share/applications/notf.desktop; rm ~/.local/share/icons/notf.png
    if (!settings.icon.empty()) {
        const std::string icon_path = app.get_arguments().texture_directory + settings.icon;
        try {
            RawImage icon(icon_path);
            if (icon.get_channels() != 4) {
                log_warning << "Icon file \"" << icon_path << "\" does not provide the required 4 byte per pixel, but "
                            << icon.get_channels();
            }
            else {
                const GLFWimage glfw_icon{icon.get_width(), icon.get_height(), const_cast<uchar*>(icon.get_data())};
                glfwSetWindowIcon(m_glfw_window.get(), 1, &glfw_icon);
                log_trace << "Loaded icon for Window \"" << get_title() << "\" from \"" << icon_path << "\"";
            }
        }
        catch (const std::runtime_error&) {
            log_warning << "Failed to load icon for Window \"" << get_title() << "\" from \"" << icon_path << "\"";
        }
    }

    log_info << "Created Window \"" << get_title() << "\" using OpenGl version: " << glGetString(GL_VERSION);
}

WindowPtr Window::_create(const Settings& settings)
{
    WindowPtr result = NOTF_MAKE_SHARED_FROM_PRIVATE(Window, settings);

    // the SceneGraph requires a fully initialized WindowPtr
    result->m_scene_graph = SceneGraph::Access<Window>::create(result);

    return result;
}

Window::~Window() { close(); }

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

void Window::request_redraw()
{
    if (TheApplication::is_running()) {
        TheApplication::get().get_render_manager().render(shared_from_this());
    }
}

void Window::set_state(const Settings::State state)
{
    { // windowify the window if it is currently in full screen
        GLFWmonitor* monitor = glfwGetWindowMonitor(m_glfw_window.get());
        if (monitor && state != Settings::State::FULLSCREEN) {
            glfwSetWindowMonitor(m_glfw_window.get(), nullptr, m_settings->pos.x(), m_settings->pos.y(),
                                 m_settings->size.width, m_settings->size.height, /*ignored*/ 0);
        }
    }

    // TODO: update Window settings
    //  monitor (whenever moved)
    //  position (whenever moved)
    //  size (whenever resized)

    switch (state) {
    case Settings::State::MINIMIZED:
        glfwIconifyWindow(m_glfw_window.get());
        break;
    case Settings::State::WINDOWED:
        glfwRestoreWindow(m_glfw_window.get());
        break;
    case Settings::State::MAXIMIZED:
        glfwMaximizeWindow(m_glfw_window.get());
        break;
    case Settings::State::FULLSCREEN: {
        GLFWmonitor* monitor = get_glfw_monitor(m_settings->monitor);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        const Size2i buffer_size = m_settings->has_buffer_size() ? m_settings->buffer_size :
                                                                   Size2i{mode->width, mode->height};
        glfwSetWindowMonitor(m_glfw_window.get(), monitor, /*ignored*/ 0, /*ignored*/ 0, buffer_size.width,
                             buffer_size.height, mode->refreshRate);
    } break;
    }
    m_settings->state = state;
}

void Window::close()
{
    if (is_closed()) {
        return;
    }
    m_is_closed.store(true, std::memory_order_release);

    log_trace << "Closing Window \"" << get_title() << "\"";

    // disconnect the window callbacks (blocks until all queued events are handled)
    EventManager::Access<Window>::remove_window(TheApplication::get().get_event_manager(), *this);

    // deletes all Nodes and Scenes in the SceneGraph before it is destroyed
    SceneGraph::Access<Window>::clear(*m_scene_graph.get());

    // remove yourself from the Application (deletes the Window if there are no more shared_ptrs to it)
    TheApplication::Access<Window>::unregister(this);
}

Window::Settings Window::_validate_settings(const Settings& given)
{
    Settings settings = given;

    // warn about ignored settings
    if (settings.state == Settings::State::FULLSCREEN) {
        if (!settings.is_visible) {
            log_warning << "Ignoring setting \"is_visible = false\" for Window \"" << settings.title
                        << "\", because the Window was requested to start in full screen";
            settings.is_visible = true;
        }
        if (settings.is_decorated) {
            log_warning << "Ignoring setting \"is_decorated = true\" for Window \"" << settings.title
                        << "\", because the Window was requested to start in full screen";
            settings.is_decorated = false;
        }
        if (!settings.is_focused) {
            log_warning << "Ignoring setting \"is_focused = false\" for Window \"" << settings.title
                        << "\", because the Window was requested to start in full screen";
            settings.is_focused = true;
        }
    }
    if (!settings.is_visible) {
        if (settings.is_focused) {
            log_warning << "Ignoring setting \"is_focused = true\" for Window \"" << settings.title
                        << "\", because the Window was requested to start invisible";
            settings.is_focused = false;
        }
    }

    if (settings.monitor != -1) { // make sure the monitor is valid
        int count = 0;
        glfwGetMonitors(&count);
        if (count == 0) {
            NOTF_THROW(Window::initialization_error, "Cannot find a monitor to place the Window on to");
        }
        if (settings.monitor < count) {
            log_warning << "Cannot place Window \"" << settings.title << "\" on monitor " << settings.monitor
                        << " because the system only provides " << count << ". Using primary monitor instead.";
            settings.monitor = 0;
        }
    }

    // correct nonsensical values
    settings.samples = max(0, settings.samples);

    return settings;
}

NOTF_CLOSE_NAMESPACE
