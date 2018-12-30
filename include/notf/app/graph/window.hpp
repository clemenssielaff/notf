#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/rational.hpp"
#include "notf/common/size2.hpp"
#include "notf/common/vector2.hpp"

#include "notf/graphic/graphics_context.hpp"

#include "notf/app/graph/scene.hpp"

NOTF_OPEN_NAMESPACE

// window arguments ================================================================================================= //

namespace detail {

/// Settings to create a Window instance.
struct WindowArguments {

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

// window policy ==================================================================================================== //

namespace window_policy {

/// Window title.
struct Title {
    using value_t = std::string;
    static constexpr ConstString name = "title";
    static inline const value_t default_value = WindowArguments().title;
    static constexpr bool is_visible = true;
};

/// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
struct Icon {
    using value_t = std::string;
    static constexpr ConstString name = "icon";
    static inline const value_t default_value = WindowArguments().icon;
    static constexpr bool is_visible = false;
};

/// Initial size when windowed.
struct Size {
    using value_t = Size2i;
    static constexpr ConstString name = "size";
    static constexpr value_t default_value = WindowArguments().size;
    static constexpr bool is_visible = true;
};

/// Position of the Window relative to the monitor's upper left corner.
/// The default (max int) means the system is free to place the Window.
struct Position {
    using value_t = V2i;
    static constexpr ConstString name = "position";
    static constexpr value_t default_value = WindowArguments().position;
    static constexpr bool is_visible = false;
};

/// Size of the Window's graphics buffer when in fullscreen.
/// The default (zero) means that the Window will assume the native screen resolution.
struct Resolution {
    using value_t = Size2i;
    static constexpr ConstString name = "resolution";
    static constexpr value_t default_value = WindowArguments().resolution;
    static constexpr bool is_visible = true;
};

/// Whether the window starts out is minimzed, windowed or maximized.
struct State {
    using value_t = WindowArguments::State;
    static constexpr ConstString name = "state";
    static constexpr value_t default_value = WindowArguments().state;
    static constexpr bool is_visible = true;
};

/// Which monitor the Window should be displayed full screen.
/// The default (-1) means that the OS is free to place the Window on whatever screen it wants to.
struct Monitor {
    using value_t = int;
    static constexpr ConstString name = "monitor";
    static constexpr value_t default_value = WindowArguments().monitor;
    static constexpr bool is_visible = false;
};

// slots ======================================================================

struct CloseSlot {
    using value_t = None;
    static constexpr ConstString name = "to_close";
};

// signals ====================================================================

struct AboutToCloseSignal {
    using value_t = None;
    static constexpr ConstString name = "on_about_to_close";
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
        >;                         //

    using slots = std::tuple< //
        CloseSlot             //
        >;                    //

    using signals = std::tuple< //
        AboutToCloseSignal      //
        >;                      //
};

} // namespace window_policy
} // namespace detail

// window =========================================================================================================== //

class Window : public Node<detail::window_policy::WindowPolicy> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Compile time Node base type.
    using super_t = Node<detail::window_policy::WindowPolicy>;

public:
    /// Settings to create a Window instance.
    using Arguments = detail::WindowArguments;

    /// System state of the Window.
    using State = Arguments::State;

    /// Windows are only allowed to parent Scenes.
    using allowed_child_types = std::tuple<Scene>;

    /// Only the Root Node may parent Windows.
    using allowed_parent_types = std::tuple<RootNode>;

    /// Property names.
    static constexpr const ConstString& title = detail::window_policy::Title::name;
    static constexpr const ConstString& icon = detail::window_policy::Icon::name;
    static constexpr const ConstString& size = detail::window_policy::Size::name;
    static constexpr const ConstString& position = detail::window_policy::Position::name;
    static constexpr const ConstString& resolution = detail::window_policy::Resolution::name;
    static constexpr const ConstString& state = detail::window_policy::State::name;
    static constexpr const ConstString& monitor = detail::window_policy::Monitor::name;

    /// Slot names.
    static constexpr const ConstString& to_close = detail::window_policy::CloseSlot::name;

private:
    /// Exception thrown when the OpenGL context of a Window could not be initialized.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(InitializationError);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(Window);

    /// Constructor.
    /// @param parent   Parent of this Node.
    /// @param window   GLFW window managed by this Window instance.
    /// @param settings Initialization settings.
    /// @throws InitializationError             If the OpenGL context creation for this Window failed
    /// @throws TheApplication::StartupError    When you try to instantiate a Window without an Application.
    Window(valid_ptr<AnyNode*> parent, valid_ptr<GLFWwindow*> window, Arguments settings);

public:
    /// Factory, creates a new Window.
    /// @param settings                         Initialization settings.
    /// @throws InitializationError             If the OpenGL context creation for this Window failed
    /// @throws TheApplication::StartupError    When you try to instantiate a Window without an Application.
    /// @throws ThreadError                     When you call this method from any thread but the UI thread.
    static WindowHandle create(Arguments settings = {});

    /// Destructor
    ~Window() override;

    /// Returns the GlfwWindow contained in this Window.
    GLFWwindow* get_glfw_window() const { return m_glfw_window; }

    /// Internal GraphicsContext.
    GraphicsContext& get_graphics_context() const { return *m_graphics_context; }

    /// Scene contained in this Window.
    SceneHandle get_scene() const { return m_scene; }

    /// (Re-)Sets the Scene displayed in this Window.
    template<class T, class... Args, class = std::enable_if_t<std::is_base_of_v<Scene, T>>>
    auto set_scene(Args&&... args) {
        _clear_children();
        auto scene_node = _create_child<T>(this, std::forward<Args>(args)...).to_handle();
        m_scene = scene_node;
        return scene_node;
    }

private:
    /// Validates (and modifies, if necessary) Settings to create a Window instance.
    /// @param settings     Given Settings, modified in-place if necessary.
    static void _validate_settings(Arguments& settings);

    static GLFWwindow* _create_glfw_window(const Arguments& settings);

    /// Moves the fullscreen Window onto the given monitor.
    /// @param window_monitor   The monitor to move the Window on.
    void _move_to_monitor(GLFWmonitor* window_monitor);

    /// Callbacks, called automatically whenever their corresponding property changed.
    bool _on_state_change(Arguments::State& new_state);
    bool _on_size_change(Size2i& new_size);
    bool _on_resolution_change(Size2i& new_resolution);
    bool _on_monitor_change(int& new_monitor);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The GLFW window managed by this Window.
    GLFWwindow* const m_glfw_window;

    /// Internal Pipes.
    AnyPipelinePtr m_pipe_to_close;

    /// Internal GraphicsContext.
    GraphicsContextPtr m_graphics_context;

    /// Scene contained in this Window.
    SceneHandle m_scene;
};

// window handle ==================================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<Window> : public NodeHandleBaseInterface<Window> {

    using Window::get_graphics_context;
    using Window::get_scene;
    using Window::set_scene;
};

} // namespace detail

class WindowHandle : public NodeHandle<Window> {

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using NodeHandle<Window>::NodeHandle;

    /// Constructor from specialized base.
    WindowHandle(NodeHandle<Window>&& handle) : NodeHandle(std::move(handle)) {}

    /// Returns the GlfwWindow contained in this Window.
    GLFWwindow* get_glfw_window() const { return _get_node()->get_glfw_window(); }
};

NOTF_CLOSE_NAMESPACE
