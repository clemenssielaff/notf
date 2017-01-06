#pragma once

#include <memory>
#include <string>

#include "common/color.hpp"
#include "common/signal.hpp"
#include "common/size2i.hpp"
#include "common/vector2.hpp"

struct GLFWwindow;
struct NVGcontext;

namespace notf {

template <typename T>
class MakeSmartEnabler;

struct KeyEvent;
class MouseEvent;
class RenderManager;
class LayoutRoot;

/**********************************************************************************************************************/

/** Destroys a GLFW window. */
void window_deleter(GLFWwindow* glfw_window);

/** Destroys a NanoVG context. */
void nanovg_deleter(NVGcontext* nvg_context);

/**********************************************************************************************************************/

/** Helper struct to create a Window instance. */
struct WindowInfo {

    /** OpenGL profiles. */
    enum class PROFILE : unsigned char {
        ANY,
        CORE,
        COMPAT,
    };

    /** Width of the window. */
    int width = 640;

    /** Height of the window. */
    int height = 480;

    /** If the Window is resizeable or not. */
    bool is_resizeable = true;

    /** If vertical synchronization is turned on or off. */
    bool enable_vsync = true;

    /** Background color of the Window. */
    Color clear_color = {0.3f, 0.3f, 0.32f, 1.0f};

    /** Window title. */
    std::string title = "NoTF";

    /** File name of the Window's icon, relative to the Application's texture directory, empty means no icon. */
    std::string icon = "";
};

/**********************************************************************************************************************/

/** The Window is a OS window containing an OpenGL context. */
class Window : public receive_signals, public std::enable_shared_from_this<Window> {

    friend class Application;
    friend class MakeSmartEnabler<Window>;

public: // methods
    /** Destructor. */
    virtual ~Window();

    /** The Window's title. */
    const std::string& get_title() const { return m_title; }

    /** The invisible root Layout of this Window. */
    std::shared_ptr<LayoutRoot> get_layout_root() const { return m_root_layout; }

    /** Returns the Application's Render Manager. */
    RenderManager& get_render_manager() { return *m_render_manager; }

    /** Returns the Window's size in screen coordinates (not pixels). */
    Size2i get_window_size() const;

    /** Returns the size of the Window including decorators added by the OS in screen coordinates (not pixels). */
    Size2i get_framed_window_size() const;

    /** Returns the size of the Window's framebuffer in pixels. */
    Size2i get_buffer_size() const;

    /** Closes this Window. */
    void close();

public: // static methods
    /** Factory function to create a new Window.
     * @param info  WindowInfo providing initialization arguments.
     * @return      The created Window, pointer is empty on error.
     */
    static std::shared_ptr<Window> create(const WindowInfo& info = WindowInfo());

public: // signals
    /** Emitted, when a single key was pressed / released / repeated. */
    Signal<const KeyEvent&> on_token_key;

    /** Emitted just before this Window is closed.
     * @param This window.
     */
    Signal<const Window&> on_close;

private: // methods for MakeSmartEnabler<Window>
    /** @param info     WindowInfo providing initialization arguments. */
    explicit Window(const WindowInfo& info);

private: // methods for Application
    /** The wrapped GLFW window. */
    GLFWwindow* _get_glwf_window() { return m_glfw_window.get(); }

    /** Called when the Window was resized. */
    void _on_resize(int width, int height);

    /** Called when the Application receives a mouse event targeting this Window. */
    void _propagate_mouse_event(MouseEvent&& event);

    /** Updates the contents of this Window. */
    void _update();

private: // fields
    /** The GLFW window managed by this Window. */
    std::unique_ptr<GLFWwindow, decltype(&window_deleter)> m_glfw_window;

    /** The NanoVG used to draw into this Window. */
    std::unique_ptr<NVGcontext, decltype(&nanovg_deleter)> m_nvg_context;

    /** The Window's title (is not accessible through GLFW). */
    std::string m_title;

    /** The Root Layout of this Window. */
    std::shared_ptr<LayoutRoot> m_root_layout;

    /** The Window's render manager. */
    std::unique_ptr<RenderManager> m_render_manager;

    /** The Window's background color. */
    Color m_background_color;

    /** Last mouse position in this Window. */
    Vector2 m_last_mouse_pos;
};

} // namespace notf
