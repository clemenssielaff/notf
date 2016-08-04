#pragma once

#include "glfw_wrapper.hpp"

#include <memory>
#include <string>

#include "common/signal.hpp"

struct GLFWwindow;

namespace signal {

struct KeyEvent;

/// \brief Helper struct to create a Window instance.
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

    /// \brief Window title.
    std::string title = "Window";
};

/// \brief Destroys a GLFW window.
void window_deleter(GLFWwindow* glfwWindow);

/// \brief The Window is a OS window containing an OpenGL context.
class Window {

    friend class Application;

public: // methods
    /// \brief Constructor.
    ///
    /// \param info WindowInfo providing initialization arguments.
    explicit Window(const WindowInfo& info = WindowInfo());

    /// \brief Destructor.
    virtual ~Window();

    /// \brief The Window's title.
    const std::string& title() const { return m_title; }

    /// \brief Closes this Window.
    void close();

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

private: // fields
    /// \brief The GLFW window managed by this Window.
    std::unique_ptr<GLFWwindow, decltype(&window_deleter)> m_glfw_window;

    /// \brief The Window's title (is not accessible through GLFW).
    std::string m_title;

    // TEMP
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    /// \brief Manager object for incoming signals.
    Callback m_callbacks;
};

} // namespace signal
