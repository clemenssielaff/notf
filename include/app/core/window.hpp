#pragma once

/*
 * previous comment about event propagation:
 * =======================================
 * Event propagation
 * =================
 * Each Window has to types of focus: the 'mouse' focus and the 'keyboard' focus.
 * The mouse focus exists only between the mouse-press and -release events and is used to make sure that a Widget will
 * always receive a corresponding -release event, as well as for allowing drag operations where the cursor might move
 * outside of the Widget's boundaries between two frames.
 * The keyboard focus is the first widget that receives key events.
 * All events are sent to a Widget first and the propagated up until some ScreenItem ancestor handles it (or not).
 * Focus events are always propagated upwards to notify the hierarchy that a child has received the focus.
 *
 * If a Window has no current keyboard item, the WindowLayout is the only one notified of a key event, for example to
 * close the Window on ESC.
 * Note that this doesn't mean that the WindowLayout is always notified!
 * If there is a keyboard item and it handles the key event, the event will not propagate further.
 * =======================================
 *
 * The window may not have a focus object, or two - one for mouse and one for keyboard.
 * Instead, we have the controller hierarchy to manage the focus.
 * This is because a Window might not even contain a Widget to begin with, or we want multiple widgest to receive
 * the events - who knows?
 */

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"
#include "graphics/forwards.hpp"

namespace notf {

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
class Window : public std::enable_shared_from_this<Window> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Private access type template.
    /// Used for finer grained friend control and is compiled away completely (if you should worry).
    template<typename T, typename = typename std::enable_if<is_one_of<T, Application>::value>::type>
    class Private;

    //================================================================================================================//

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

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted just before this Window is closed.
    /// @param This Window.
    Signal<const Window&> on_close;

    /// Emitted when the mouse cursor entered the client area of this Window.
    /// @param This Window.
    Signal<const Window&> on_cursor_entered;

    /// Emitted when the mouse cursor exited the client area of this Window.
    /// @param This Window.
    Signal<const Window&> on_cursor_exited;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param args     Initialization arguments.
    /// @throws window_initialization_error         If the OpenGL context creation for this Window failed
    /// @throws application_initialization_error    When you try to instantiate a Window without an Application.
    Window(const Args& args);

public:
    NO_COPY_AND_ASSIGN(Window)

    /// Factory.
    /// @param info  WindowInfo providing initialization arguments.
    /// @return      The created Window, pointer is empty on error.
    static WindowPtr create(const Args& info = s_default_args);

    /// Destructor.
    virtual ~Window();

    /// The Window's title.
    const std::string& title() const { return m_title; }

    /// The invisible root Layout of this Window.
    WindowLayoutPtr layout() const { return m_layout; }

    /// Returns the Application's Render Manager.
    RenderManager& render_manager() { return *m_render_manager; }

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

    /// Requests a redraw of this Window at the next possibility.
    void request_redraw() const;

    /// Closes this Window.
    void close();

    /// Returns false if the GLFW window is still open or true if it was closed.
    bool is_closed() const { return !(static_cast<bool>(m_glfw_window)); }

private:
    /// Called by the Application when the Window was resized.
    /// @param size     New size.
    void _resize(Size2i size);

    /// Called when the Application receives a mouse event targeting this Window.
    /// @param event    MouseEvent.
    void _propagate(MouseEvent&& event);

    /// Called when the Application receives a key event targeting this Window.
    /// @param event    KeyEvent.
    void _propagate(KeyEvent&& event);

    /// Called when the Application receives a character input event targeting this Window.
    /// @param event    CharEvent.
    void _propagate(CharEvent&& event);

    /// Updates the contents of this Window.
    void _update();

    /// The wrapped GLFW window.
    GLFWwindow* _glfw_window() const { return m_glfw_window.get(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&detail::window_deleter)> m_glfw_window;

    /// The Window's title (is not accessible through GLFW).
    std::string m_title;

    /// The Root Layout of this Window.
    WindowLayoutPtr m_layout;

    /// The Window's render manager.
    std::unique_ptr<RenderManager> m_render_manager;

    /// The Window size.
    Size2i m_size;

    /// Default arguments, when the user didn't supply any.
    static const Args s_default_args;
};

// ===================================================================================================================//

template<>
class Window::Private<Application> {
    friend class Application;

    /// Constructor.
    /// @param window   Window to access.
    Private(Window& window) : m_window(window) {}

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    MouseEvent.
    void propagate(MouseEvent&& event) { m_window._propagate(std::move(event)); }

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    KeyEvent.
    void propagate(KeyEvent&& event) { m_window._propagate(std::move(event)); }

    /// Forwards an event generated by the Application to be handled by this Window.
    /// @param event    CharEvent.
    void propagate(CharEvent&& event) { m_window._propagate(std::move(event)); }

    /// Notify the Window, that it was resized.
    /// @param size     New size.
    void resize(Size2i size) { m_window._resize(std::move(size)); }

    /// The GLFW window wrapped by the NoTF Window.
    GLFWwindow* glfw_window() { return m_window._glfw_window(); }

    /// The Window to access.
    Window& m_window;
};

} // namespace notf
