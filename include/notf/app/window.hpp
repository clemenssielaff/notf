#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/rational.hpp"
#include "notf/common/size2.hpp"
#include "notf/common/vector2.hpp"

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// window settings ================================================================================================== //

namespace detail {

/// Settings to create a Window instance.
struct WindowSettings {

    /// Window title.
    const char* title = "notf";

    /// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
    const char* icon = "";

    /// Size of the Window when windowed.
    Size2i size = {640, 480};

    /// Size of the Window's graphics buffer when in fullscreen.
    /// When windowed, the resolution will correspond to the Window's size instead.
    /// The default (zero) means that the Window will assume the native screen resolution.
    Size2i resolution = Size2i::zero();

    /// Position of the Window relative to the monitor's upper left corner.
    /// The default (max int) means the system is free to place the Window.
    V2i position = {max_value<int>(), max_value<int>()};

    /// Whether the window starts out is minimzed, windowed or maximized.
    enum class State {
        MINIMIZED,  //< Window is minimized to the task bar.
        WINDOWED,   //< Window is movable on the screen.
        MAXIMIZED,  //< Window is maximized.
        FULLSCREEN, //< Window takes up the entire screen without additional border and decorations.
    } state{State::WINDOWED};

    /// Which monitor the Window should be displayed full screen.
    /// The default (-1) means that the OS is free to place the Window on whatever screen it wants to.
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
};

namespace window_properties {

// properties =================================================================

/// Window title.
struct Title {
    using value_t = std::string;
    static constexpr StringConst name = "title";
    static inline const value_t default_value = WindowSettings().title;
    static constexpr bool is_visible = true;
};

/// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
struct Icon {
    using value_t = std::string;
    static constexpr StringConst name = "icon";
    static inline const value_t default_value = WindowSettings().icon;
    static constexpr bool is_visible = false;
};

/// Initial size when windowed.
struct Size {
    using value_t = Size2i;
    static constexpr StringConst name = "size";
    static constexpr value_t default_value = WindowSettings().size;
    static constexpr bool is_visible = true;
};

/// Position of the Window relative to the monitor's upper left corner.
/// The default (max int) means the system is free to place the Window.
struct Position {
    using value_t = V2i;
    static constexpr StringConst name = "position";
    static constexpr value_t default_value = WindowSettings().position;
    static constexpr bool is_visible = false;
};

/// Size of the Window's graphics buffer when in fullscreen.
/// The default (zero) means that the Window will assume the native screen resolution.
struct Resolution {
    using value_t = Size2i;
    static constexpr StringConst name = "resolution";
    static constexpr value_t default_value = WindowSettings().resolution;
    static constexpr bool is_visible = true;
};

/// Whether the window starts out is minimzed, windowed or maximized.
struct State {
    using value_t = WindowSettings::State;
    static constexpr StringConst name = "state";
    static constexpr value_t default_value = WindowSettings().state;
    static constexpr bool is_visible = true;
};

/// Which monitor the Window should be displayed full screen.
/// The default (-1) means that the OS is free to place the Window on whatever screen it wants to.
struct Monitor {
    using value_t = int;
    static constexpr StringConst name = "monitor";
    static constexpr value_t default_value = WindowSettings().monitor;
    static constexpr bool is_visible = false;
};

// slots ======================================================================

struct CloseSlot {
    using value_t = None;
    static constexpr StringConst name = "to_close";
};

// signals ====================================================================

struct AboutToCloseSignal {
    using value_t = None;
    static constexpr StringConst name = "on_about_to_close";
};

// policy ===================================================================

struct WindowPolicy {
    using properties = std::tuple< //
        Title,                     //
        Icon,                      //
        Size,                      //
        Position,                  //
        Resolution,                //
        State,                     //
        Monitor                    //
        >;

    using slots = std::tuple< //
        CloseSlot             //
        >;                    //

    using signals = std::tuple< //
        AboutToCloseSignal      //
        >;                      //
};

} // namespace window_properties
} // namespace detail

// window =========================================================================================================== //

class Window : public CompileTimeNode<detail::window_properties::WindowPolicy> {

    friend class WindowHandle;

    // types ----------------------------------------------------------------------------------- //
private:
    /// Compile time Node base type.
    using super_t = CompileTimeNode<detail::window_properties::WindowPolicy>;

public:
    /// Settings to create a Window instance.
    using Settings = detail::WindowSettings;

    /// System state of the Window.
    using State = Settings::State;

    /// Property names.
    static constexpr const StringConst& title = detail::window_properties::Title::name;
    static constexpr const StringConst& icon = detail::window_properties::Icon::name;
    static constexpr const StringConst& size = detail::window_properties::Size::name;
    static constexpr const StringConst& position = detail::window_properties::Position::name;
    static constexpr const StringConst& resolution = detail::window_properties::Resolution::name;
    static constexpr const StringConst& state = detail::window_properties::State::name;
    static constexpr const StringConst& monitor = detail::window_properties::Monitor::name;

    /// Slot names.
    static constexpr const StringConst& to_close = detail::window_properties::CloseSlot::name;

private:
    /// Exception thrown when the OpenGL context of a Window could not be initialized.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(initialization_error);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(Window);

    /// Constructor.
    /// @param parent                               Parent of this Node.
    /// @param settings                             Initialization settings.
    /// @throws initialization_error                If the OpenGL context creation for this Window failed
    /// @throws Application::initialization_error   When you try to instantiate a Window without an Application.
    Window(valid_ptr<Node*> parent, Settings settings);

public:
    /// Factory, creates a new Window.
    /// @param settings                             Initialization settings.
    /// @throws initialization_error                If the OpenGL context creation for this Window failed
    /// @throws Application::initialization_error   When you try to instantiate a Window without an Application.
    static WindowHandle create(Settings settings = {});

private:
    /// Closes this Window.
    void _close();

    /// Validates (and modifies, if necessary) Settings to create a Window instance.
    /// @param settings     Given Settings, modified in-place if necessary.
    static void _validate_settings(Settings& settings);

    /// Moves the fullscreen Window onto the given monitor.
    /// @param window_monitor   The monitor to move the Window on.
    void _move_to_monitor(GLFWmonitor* window_monitor);

    /// Callbacks, called automatically whenever their corresponding property changed.
    bool _on_state_change(Settings::State& new_state);
    bool _on_size_change(Size2i& new_size);
    bool _on_resolution_change(Size2i& new_resolution);
    bool _on_monitor_change(int& new_monitor);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The GLFW window managed by this Window.
    detail::GlfwWindowPtr m_glfw_window;

    /// Internal GraphicsContext.
    //    GraphicsContextPtr m_graphics_context;
};

// window handle ==================================================================================================== //

class WindowHandle : public TypedNodeHandle<Window> {
//    friend class Window;

    friend Accessor<WindowHandle, TheApplication>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(WindowHandle);

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor
    WindowHandle(WindowPtr window) : TypedNodeHandle<Window>(std::move(window)) {}

    /// The Window's title.
    std::string get_title() const { return _get_node()->get<Window::title>(); }

    /// Closes the Window.
    void close() { _get_node()->get_slot<Window::to_close>().call(); }

private:
    /// Returns the GlfwWindow contained in a Window.
    GLFWwindow* _get_glfw_window() { return _get_node()->m_glfw_window.get(); }
};

// accessors ======================================================================================================== //

template<>
class Accessor<WindowHandle, TheApplication> {
    friend TheApplication;

    /// Returns the GlfwWindow contained in a Window.
    static GLFWwindow* get_glfw_window(WindowHandle& window) { return window._get_glfw_window(); }
};

NOTF_CLOSE_NAMESPACE
