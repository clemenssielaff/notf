#pragma once

#include "app/application.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Exception thrown when the OpenGL context of a Window could not be initialized.
/// The error string will contain more detailed information about the error.
NOTF_EXCEPTION_TYPE(window_initialization_error)

//====================================================================================================================//

namespace detail {

/// Destroys a GLFW window.
void window_deleter(GLFWwindow* glfw_window);

} // namespace detail

//====================================================================================================================//

/// The Window is a OS window containing an OpenGL context.
class Window {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Application)

    /// Whether the window is minimzed, windowed or maxmized.
    enum class State {
        MINIMIZED,  //< Window is minimized to the task bar.
        WINDOWED,   //< Window is visible on the screen with border and decorations.
        FULLSCREEN, //< Window takes up the entire screen without additional border and decorations.
    };

    /// Helper struct to create a Window instance.
    using Args = detail::WindowArguments;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    /// @param args                                 Initialization arguments.
    /// @throws window_initialization_error         If the OpenGL context creation for this Window failed
    /// @throws application_initialization_error    When you try to instantiate a Window without an Application.
    Window(const Args& args);

    /// Factory.
    /// @param args     Initialization arguments.
    /// @throws window_initialization_error         If the OpenGL context creation for this Window failed
    /// @throws application_initialization_error    When you try to instantiate a Window without an Application.
    static WindowPtr _create(const Args& args);

public:
    NOTF_NO_COPY_OR_ASSIGN(Window)

    /// Destructor.
    virtual ~Window();

    /// The Window's title.
    const std::string& title() const { return m_title; }

    ///@{
    /// Internal GraphicsContext.
    GraphicsContext& graphics_context() { return *m_graphics_context; }
    const GraphicsContext& graphics_context() const { return *m_graphics_context; }
    ///@}

    ///@{
    /// Scenes displayed in the Window.
    SceneManager& scene_manager() { return *m_scene_manager; }
    const SceneManager& scene_manager() const { return *m_scene_manager; }
    ///@}

    ///@{
    /// FontManager used to render text.
    FontManager& font_manager() { return *m_font_manager; }
    const FontManager& font_manager() const { return *m_font_manager; }
    ///@}

    /// Returns the Window's size in screen coordinates (not pixels).
    /// Returns an invalid size if the GLFW window was already closed.
    Size2i window_size() const { return m_size; }

    /// Returns the size of the Window including decorators added by the OS in screen coordinates (not pixels).
    /// Returns an invalid size if the GLFW window was already closed.
    Size2i framed_window_size() const;

    /// Returns the size of the Window's framebuffer in pixels.
    /// Returns an invalid size if the GLFW window was already closed.
    Size2i buffer_size() const;

    /// Returns the position of the mouse pointer relativ to the Window's top-left corner in screen coordinates.
    /// Returns zero if the GLFW window was already closed.
    Vector2f mouse_pos() const;

    /// Tell the RenderManager to redraw this Window at the next opportunity.
    void request_redraw();

    /// Sets the current state of the Window.
    /// If the new state is equal to the old one this method does nothing.
    /// @param state    New state.
    void set_state(const State state);
    // TODO: connect more glfw window callbacks

    /// Closes this Window.
    void close();

    /// Returns false if the GLFW window is still open or true if it was closed.
    bool is_closed() const { return !(static_cast<bool>(m_glfw_window)); }

private:
    /// The wrapped GLFW window.
    GLFWwindow* _glfw_window() { return m_glfw_window.get(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&detail::window_deleter)> m_glfw_window;

    /// Internal GraphicsContext.
    GraphicsContextPtr m_graphics_context;

    /// Scenes displayed in the Window.
    SceneManagerPtr m_scene_manager;

    /// FontManager used to render text.
    FontManagerPtr m_font_manager; // TODO: FontManager per Window doesn't seem right

    /// The Window's title (is not accessible through GLFW).
    std::string m_title;

    /// Current state of the window.
    State m_state;

    /// The Window size.
    Size2i m_size;

    /// Default arguments, when the user didn't supply any.
    static const Args s_default_args;
};

// ===================================================================================================================//

template<>
class Window::Access<Application> {
    friend class Application;

    /// Factory.
    /// @param args     Initialization arguments.
    /// @throws window_initialization_error         If the OpenGL context creation for this Window failed
    /// @throws application_initialization_error    When you try to instantiate a Window without an Application.
    static WindowPtr create(const Args& args = Window::s_default_args) { return Window::_create(args); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Window that was granted access.
    Window* m_window;
};

//====================================================================================================================//

namespace detail {

///// Helper struct to create a Window instance.
struct WindowArguments {

    /// Window title.
    std::string title = "NoTF";

    /// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
    std::string icon = "";

    /// Initial size of the Window.
    Size2i size = {640, 480};

    /// If the Window is resizeable or not.
    bool is_resizeable = true;

    /// Initial state of the Window.
    Window::State state = Window::State::WINDOWED;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE
