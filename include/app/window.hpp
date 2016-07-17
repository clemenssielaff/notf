#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "common/keys.hpp"

struct GLFWwindow;

namespace untitled {

///
/// \brief Helper struct to create a Window instance.
///
struct WindowInfo {

    ///
    /// \brief Window title.
    ///
    std::string title = "Window";

    ///
    /// \brief Width of the window.
    ///
    int width = 640;

    ///
    /// \brief Height of the window.
    ///
    int height = 480;

    ///
    /// \brief Minimum required OpenGL minimum version number (-1 = no minimum)
    ///
    int opengl_version_minor = -1;

    ///
    /// \brief Minimum required OpenGL major version number (-1 = no minimum)
    ///
    int opengl_version_major = -1;
};

///
/// \brief The Window is a OS window containing an OpenGL context.
///
class Window {

    friend class Application; // can create/destroy Windows and has access to the GLFW window

public: // methods
    ///
    /// \brief The Window's title.
    ///
    /// \return The Window's title.
    ///
    const std::string& get_title() const { return m_title; }

    ///
    /// \brief Checks if the window should close.
    ///
    /// \return Returns true, iff the Window should close after the next main loop.
    ///
    bool is_closing() const { return glfwWindowShouldClose(m_glfw_window); }

private: // methods for Application
    ///
    /// \brief Value constructor.
    ///
    /// \param info WindowInfo providing initialization arguments.
    ///
    explicit Window(const WindowInfo& info = WindowInfo());

    ///
    /// \brief Destructor.
    ///
    ~Window();

    ///
    /// \brief The wrapped GLFW window.
    ///
    /// \return The wrapped GLFW window.
    ///
    GLFWwindow* get_glwf_window() { return m_glfw_window; }

    ///
    /// \brief Updates the contents of this Window.
    ///
    void update();

private: // callbacks for Application
    ///
    /// \brief Callback when a key was pressed, repeated or released.
    ///
    /// \param key      Modified key.
    /// \param action   The key action that triggered this callback.
    /// \param mods     Additional modifier keys.
    ///
    void on_token_key(KEY key, KEY_ACTION action, KEY_MODS mods);

private: // methods
    ///
    /// \brief Closes the window after the next mainloop.
    ///
    void close() { glfwSetWindowShouldClose(m_glfw_window, true); }

private: // fields
    ///
    /// \brief The GLFW window managed by this Window.
    ///
    GLFWwindow* m_glfw_window;

    ///
    /// \brief The Window's title (is not accessible through GLFW).
    ///
    std::string m_title;
};

} // namespace untitled
