#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

namespace detail {

/// Helper struct to create a Window instance.
struct WindowArguments {

    /// Initial size of the Window.
    Size2i size = {640, 480};

    /// If the Window is resizeable or not.
    bool is_resizeable = true;

    /// Window title.
    std::string title = "NoTF";

    /// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
    std::string icon = "";
};

} // namespace detail

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

public:
    NOTF_NO_COPY_OR_ASSIGN(Window)

    /// Factory.
    /// @param args     Initialization arguments.
    /// @throws window_initialization_error         If the OpenGL context creation for this Window failed
    /// @throws application_initialization_error    When you try to instantiate a Window without an Application.
    static WindowPtr create(const Args& args = s_default_args);

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
    /// Layers displayed in the Window.
    std::vector<LayerPtr>& layers() { return m_layers; }
    const std::vector<LayerPtr>& layers() const { return m_layers; }
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

    /// Renders a single frame with the current State of the SceneManager.
    void request_redraw() const;

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

    /// Render thread.
    RenderThreadPtr m_render_thread;

    /// Layers displayed in the Window.
    std::vector<LayerPtr> m_layers;

    /// FontManager used to render text.
    FontManagerPtr m_font_manager; // TODO: FontManager per Window doesn't seem right

    /// The Window's title (is not accessible through GLFW).
    std::string m_title;

    /// The Window size.
    Size2i m_size;

    /// Default arguments, when the user didn't supply any.
    static const Args s_default_args;
};

NOTF_CLOSE_NAMESPACE
