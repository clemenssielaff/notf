#pragma once

#include <memory>
#include <string>

#include "common/color.hpp"
#include "common/handle.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"

struct GLFWwindow;

namespace signal {

struct KeyEvent;
class RenderManager;
class RootLayoutItem;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Destroys a GLFW window.
void window_deleter(GLFWwindow* glfwWindow);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Helper struct to create a Window instance.
///
struct WindowInfo {

    /// \brief Width of the window.
    int width = 640;

    /// \brief Height of the window.
    int height = 480;

    /// \brief Minimum required OpenGL minimum version number (-1 = no minimum)
    int opengl_version_minor = -1;

    /// \brief Minimum required OpenGL major version number (-1 = no minimum)
    int opengl_version_major = -1;

    /// \brief If the Window is resizeable or not.
    bool is_resizeable = true;

    /// \brief Number of samples for multisampling, <=0 disables multisampling
    int samples = 2;

    /// \brief If vertical synchronization is turned on or off.
    bool enable_vsync = true;

    /// \brief Background color of the Window.
    Color clear_color = {0.2f, 0.3f, 0.3f, 1.0f};

    /// \brief Window title.
    std::string title = "Window";

    /// \brief Handle of this Window's root Widget (BAD_HANDLE means that a new Handle is assigned).
    Handle root_widget_handle = BAD_HANDLE;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief The Window is a OS window containing an OpenGL context.
class Window : public std::enable_shared_from_this<Window> {

    friend class Application;

public: // methods
    /// \brief Destructor.
    virtual ~Window();

    /// \brief The Window's title.
    const std::string& get_title() const { return m_title; }

    /// \brief The invisible root widget of this Window.
    std::shared_ptr<RootLayoutItem> get_root_widget() const { return m_root_widget; }

    /// \brief Returns the Application's Render Manager.
    RenderManager& get_render_manager() { return *m_render_manager; }

    /// \brief Returns the Window's size in screen coordinates (not pixels).
    Size2 get_window_size() const;

    /// \brief Returns the size of the Window including decorators added by the OS in screen coordinates (not pixels).
    Size2 get_framed_window_size() const;

    /// \brief Returns the size of the canvas displayed in this Window in pixels.
    Size2 get_canvas_size() const;

    /// \brief Closes this Window.
    void close();

public: // static methods
    /// \brief Factory function to create a new Window.
    /// \param info WindowInfo providing initialization arguments.
    /// \return The created Window, pointer is empty on error.
    static std::shared_ptr<Window> create(const WindowInfo& info = WindowInfo());

public: // signals
    /// \brief Emitted, when a single key was pressed / released / repeated.
    ///
    /// \param The event object.
    Signal<const KeyEvent&> on_token_key;

    /// \brief Emitted just before this Window is closed.
    ///
    /// \param This window.
    Signal<const Window&> on_close;

private: // methods for Application
    /// \brief The wrapped GLFW window.
    GLFWwindow* glwf_window() { return m_glfw_window.get(); }

    /// \brief Updates the contents of this Window.
    void update();

protected: // methods
    /// \brief Value Constructor.
    /// \param info WindowInfo providing initialization arguments.
    explicit Window(const WindowInfo& info);

private: // fields
    /// \brief The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&window_deleter)> m_glfw_window;

    /// \brief The Window's title (is not accessible through GLFW).
    std::string m_title;

    /// \brief The invisible root widget of this Window.
    std::shared_ptr<RootLayoutItem> m_root_widget;

    /// \brief The Window's render manager.
    std::unique_ptr<RenderManager> m_render_manager;

    CALLBACKS(Window)
};

} // namespace signal
