#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"
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

    /// Helper struct to create a Window instance.
    struct Args {

        /// Initial size of the Window.
        Size2i size = {640, 480};

        /// If the Window is resizeable or not.
        bool is_resizeable = true;

        /// Window title.
        std::string title = "NoTF";

        /// File name of the Window's icon, relative to the Application's texture directory, empty means no icon.
        std::string icon = "";
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param args     Initialization arguments.
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

    /// Returns the Application's SceneManager.
    SceneManager& scene_manager() { return *m_scene_manager; }

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

    /// Closes this Window.
    void close();

    /// Returns false if the GLFW window is still open or true if it was closed.
    bool is_closed() const { return !(static_cast<bool>(m_glfw_window)); }

private:
    /// Called when the Application receives a mouse event targeting this Window.
    /// @param event    MouseEvent.
    void _propagate(MouseEvent&& event);

    /// Called when the Application receives a key event targeting this Window.
    /// @param event    KeyEvent.
    void _propagate(KeyEvent&& event);

    /// Called when the Application receives a character input event targeting this Window.
    /// @param event    CharEvent.
    void _propagate(CharEvent&& event);

    /// Called when the cursor enters or exits the Window's client area or the window is about to be closed.
    /// @param event    WindowEvent.
    void _propagate(WindowEvent&& event);

    /// Called by the Application when the Window was resized.
    /// @param size     New size.
    void _resize(Size2i size);

    /// The wrapped GLFW window.
    GLFWwindow* _glfw_window() const { return m_glfw_window.get(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&detail::window_deleter)> m_glfw_window;

    /// The Window's title (is not accessible through GLFW).
    std::string m_title;

    /// The Window's Scene manager.
    SceneManagerPtr m_scene_manager;

    /// The Window size.
    Size2i m_size;

    /// Default arguments, when the user didn't supply any.
    static const Args s_default_args;
};

// ===================================================================================================================//

template<>
class Window::Access<Application> {
    friend class Application;

    /// Constructor.
    /// @param window   Window to access.
    Access(Window& window) : m_window(window) {}

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    MouseEvent.
    void propagate(MouseEvent&& event) { m_window._propagate(std::move(event)); }

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    KeyEvent.
    void propagate(KeyEvent&& event) { m_window._propagate(std::move(event)); }

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    CharEvent.
    void propagate(CharEvent&& event) { m_window._propagate(std::move(event)); }

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    WindowEvent.
    void propagate(WindowEvent&& event) { m_window._propagate(std::move(event)); }

    /// Notify the Window, that it was resized.
    /// @param size     New size.
    void resize(Size2i size) { m_window._resize(std::move(size)); }

    /// The GLFW window wrapped by the NoTF Window.
    GLFWwindow* glfw_window() { return m_window._glfw_window(); }

    /// The Window to access.
    Window& m_window;
};

NOTF_CLOSE_NAMESPACE
