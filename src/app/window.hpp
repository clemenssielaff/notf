#pragma once

#include "app/application.hpp"
#include "common/rational.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _Window;
} // namespace access

// ================================================================================================================== //

namespace detail {

/// Destroys a GLFW window.
void window_deleter(GLFWwindow* glfw_window);

/// Settings to create a Window instance.
struct WindowSettings {

    /// Window title.
    std::string title = "NoTF";

    /// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
    std::string icon = "";

    /// Initial size when windowed.
    Size2i size = {640, 480};

    /// Minimum size of the Window.
    Size2i min_size = {0, 0};

    /// Maximum size of the Window.
    Size2i max_size = {max_value<int>(), max_value<int>()};

    /// The aspect ratio of the Window. The default (zero) means no constraint.
    Rationali aspect_ratio = Rationali::zero();

    /// Position of the Window relative to the monitor's upper left corner.
    /// The default (max int) means the system is free to place the Window.
    Vector2i pos = {max_value<int>(), max_value<int>()};

    /// Size of the Window's graphics buffer when in fullscreen.
    /// The default (zero) means that the Window will assume the native screen resolution.
    Size2i buffer_size = Size2i::zero();

    /// Whether the window starts out is minimzed, windowed or maxmized.
    enum class State {
        MINIMIZED,  //< Window is minimized to the task bar.
        WINDOWED,   //< Window is movable on the screen.
        MAXIMIZED,  //< Window is maximized.
        FULLSCREEN, //< Window takes up the entire screen without additional border and decorations.
    } state{State::WINDOWED};

    /// Which monitor the Window should be placed on.
    /// When the Window is set to full screen, this is the monitor that it will occupy.
    /// The default (-1) means that the OS is free to place the Window on whatever screen it wants to.
    /// In that case, when setting a Window full screen, it will occupy the last monitor that the Window was on.
    int monitor = -1;

    /// Samples used for multisampling. Zero disables multisampling.
    int samples = 0;

    /// Whether the Window will be visible initially. Is ignored if `state == FULLSCREEN`.
    bool is_visible = true;

    /// Whether the Window will be given focus upon creation. Is ignored if `start_visible` is set to false.
    bool is_focused = true;

    /// Whether the Window should have OS-supplied decorations such as a border, a title bar etc.
    /// Is ignored if `state == FULLSCREEN`.
    bool is_decorated = true;

    /// If the Window is resizeable or not. Is ignored if `state == FULLSCREEN`.
    bool is_resizeable = true;

    /// Tests whether the settings specify a minimum Window size.
    bool has_min() const { return !min_size.is_zero(); }

    /// Tests whether the settings specify a maximum Window size.
    bool has_max() const { return max_size != Size2i{max_value<int>(), max_value<int>()}; }

    /// Tests whether the settings specify a aspect ratio constraint on the Window.
    bool has_aspect_ratio() const { return aspect_ratio != Rationali::zero(); }

    /// Tests whether the settings specify a buffer size.
    bool has_buffer_size() const { return buffer_size == Size2i::zero(); }

    /// Tests whether the settings specify a monitor.
    bool has_monitor() const { return monitor != -1; }
};

} // namespace detail

// ================================================================================================================== //

/// The Window is a OS window containing an OpenGL context.
class Window : public std::enable_shared_from_this<Window> {

    friend class access::_Window<Application>;
    friend class access::_Window<EventManager>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Window<T>;

    /// Helper struct to create a Window instance.
    using Settings = detail::WindowSettings;

    /// Exception thrown when the OpenGL context of a Window could not be initialized.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(initialization_error);

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param settings                             Initialization settings.
    /// @throws initialization_error                If the OpenGL context creation for this Window failed
    /// @throws Application::initialization_error   When you try to instantiate a Window without an Application.
    Window(const Settings& settings);

    /// Factory.
    /// @param settings                             Initialization settings.
    /// @throws initialization_error                If the OpenGL context creation for this Window failed
    /// @throws Application::initialization_error   When you try to instantiate a Window without an Application.
    static WindowPtr _create(const Settings& settings);

public:
    NOTF_NO_COPY_OR_ASSIGN(Window);

    /// Destructor.
    virtual ~Window();

    /// The Window's title.
    const std::string& get_title() const { return m_settings->title; }

    ///@{
    /// Internal GraphicsContext.
    GraphicsContext& get_graphics_context() { return *m_graphics_context; }
    const GraphicsContext& get_graphics_context() const { return *m_graphics_context; }
    ///@}

    ///@{
    /// Scenes displayed in the Window.
    SceneGraphPtr& get_scene_graph() { return m_scene_graph; }
    const SceneGraphPtr& get_scene_graph() const { return m_scene_graph; }
    ///@}

    /// Position of the upper-left corner of the Window in screen coordinates.
    Vector2i get_position() const { return m_settings->pos; }

    /// Returns the size of the Window including decorators added by the OS in screen coordinates (not pixels).
    /// Returns an invalid size if the GLFW window was already closed.
    Size2i get_framed_window_size() const;

    /// Returns the size of the Window's framebuffer in pixels.
    /// Returns an invalid size if the GLFW window was already closed.
    Size2i get_buffer_size() const;

    /// Returns the position of the mouse pointer relativ to the Window's top-left corner in screen coordinates.
    /// Returns zero if the GLFW window was already closed.
    Vector2f get_mouse_pos() const;

    /// Tell the RenderManager to redraw this Window at the next opportunity.
    void request_redraw();

    /// Sets the current state of the Window.
    /// If the new state is equal to the old one this method does nothing.
    /// @param state    New state.
    void set_state(Settings::State state);

    /// Closes this Window.
    void close();

    /// Returns false if the GLFW window is still open or true if it was closed.
    bool is_closed() const { return m_is_closed.load(std::memory_order_acquire); }

private:
    /// The wrapped GLFW window.
    GLFWwindow* _get_glfw_window() { return m_glfw_window.get(); }

    /// Validates and returns valid Settings to create a Window instance.
    static Settings _validate_settings(const Settings& given);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&detail::window_deleter)> m_glfw_window;

    /// Internal GraphicsContext.
    GraphicsContextPtr m_graphics_context;

    /// Scenes displayed in the Window.
    SceneGraphPtr m_scene_graph;

    /// Window settings.
    std::unique_ptr<Settings> m_settings;

    /// Flag to indicate whether the Window is still open or already closed.
    std::atomic<bool> m_is_closed{false};

    /// Default arguments, when the user didn't supply any.
    static const Settings s_default_settings;
};

// ================================================================================================================== //

template<>
class access::_Window<Application> {
    friend class notf::Application;

    /// Factory.
    /// @param args     Initialization arguments.
    /// @throws initialization_error                If the OpenGL context creation for this Window failed
    /// @throws Application::initialization_error   When you try to instantiate a Window without an Application.
    static WindowPtr create(const Window::Settings& args = Window::s_default_settings) { return Window::_create(args); }
};

// ================================================================================================================== //

template<>
class access::_Window<EventManager> {
    friend class notf::EventManager;

    /// The GLFW window wrapped by the accessed notf Window.
    /// @param window   Window granting access.
    static GLFWwindow* glfw_window(Window& window) { return window.m_glfw_window.get(); }
};

NOTF_CLOSE_NAMESPACE
