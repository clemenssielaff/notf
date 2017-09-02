#pragma once

#include <memory>
#include <string>

#include "common/color.hpp"
#include "common/signal.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

struct GLFWwindow;

namespace notf {

class CharEvent;
class KeyEvent;
class CellCanvas;
class Widget;
class WindowLayout;
class MouseEvent;
class GraphicsContext;
class RenderManager;

using WindowLayoutPtr = std::shared_ptr<WindowLayout>;

/**********************************************************************************************************************/

/** Destroys a GLFW window. */
void window_deleter(GLFWwindow* glfw_window);

/**********************************************************************************************************************/

/** Helper struct to create a Window instance. */
struct WindowInfo {

    /** Initial size of the Window. */
    Size2i size = {640, 480};

    /** If the Window is resizeable or not. */
    bool is_resizeable = true;

    /** Background color of the Window. */
    Color clear_color = {0.3f, 0.3f, 0.32f, 1.0f};

    /** Window title. */
    std::string title = "NoTF";

    /** File name of the Window's icon, relative to the Application's texture directory, empty means no icon. */
    std::string icon = "";
};

/**********************************************************************************************************************/

/** The Window is a OS window containing an OpenGL context.
 *
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
 */
class Window : public receive_signals, public std::enable_shared_from_this<Window> {

    friend class Application;
    friend class GraphicsContext;

public: // static methods
    /** Factory function to create a new Window.
     * @param info  WindowInfo providing initialization arguments.
     * @return      The created Window, pointer is empty on error.
     */
    static std::shared_ptr<Window> create(const WindowInfo& info = WindowInfo());

protected: // constructor.
    /** @param info     WindowInfo providing initialization arguments. */
    Window(const WindowInfo& info);

public: // methods
    /** Destructor. */
    virtual ~Window();

    /** The Window's title. */
    const std::string& get_title() const { return m_title; }

    /** The invisible root Layout of this Window. */
    WindowLayoutPtr get_layout() const { return m_layout; }

    /** Returns the Application's Render Manager. */
    RenderManager& get_render_manager() { return *m_render_manager; }

    /** Returns the GraphicsContext associated with this Window. */
    GraphicsContext& get_graphics_context() const { return *m_graphics_context; }

    /** Object drawing Cells into the Window. */
    CellCanvas& get_cell_canvas() const { return *m_cell_canvas; }

    /** Returns the Window's size in screen coordinates (not pixels).
     * Returns an invalid size if the GLFW window was already closed.
     */
    Size2i get_window_size() const { return m_size; }

    /** Returns the size of the Window including decorators added by the OS in screen coordinates (not pixels).
     * Returns an invalid size if the GLFW window was already closed.
    */
    Size2i get_framed_window_size() const;

    /** Returns the size of the Window's framebuffer in pixels.
     * Returns an invalid size if the GLFW window was already closed.
     */
    Size2i get_buffer_size() const;

    /** Returns the position of the mouse pointer relativ to the Window's top-left corner in screen coordinates.
     * Returns zero if the GLFW window was already closed.
     */
    Vector2f get_mouse_pos() const;

    /** Closes this Window. */
    void close();

    /** Returns false if the GLFW window is still open or true if it was closed. */
    bool was_closed() const { return !(static_cast<bool>(m_glfw_window)); }

public: // signals
    /** Emitted just before this Window is closed.
     * @param   This Window.
     */
    Signal<const Window&> on_close;

    /** Emitted when the mouse cursor entered the client area of this Window.
     * @param   This Window.
     */
    Signal<const Window&> on_cursor_entered;

    /** Emitted when the mouse cursor exited the client area of this Window.
     * @param   This Window.
     */
    Signal<const Window&> on_cursor_exited;

private: // methods for friends
    /** Called when the Window was resized. */
    void _on_resize(int width, int height);

    /** Called when the Application receives a mouse event targeting this Window. */
    void _propagate_mouse_event(MouseEvent&& event);

    /** Called when the Application receives a key event targeting this Window. */
    void _propagate_key_event(KeyEvent&& event);

    /** Called when the Application receives a character input event targeting this Window. */
    void _propagate_char_event(CharEvent&& event);

    /** Updates the contents of this Window. */
    void _update();

    /** The wrapped GLFW window. */
    GLFWwindow* _get_glfw_window() const { return m_glfw_window.get(); }

private: // fields
    /** The GLFW window managed by this Window. */
    std::unique_ptr<GLFWwindow, decltype(&window_deleter)> m_glfw_window;

    /** The Window's title (is not accessible through GLFW). */
    std::string m_title;

    /** The Root Layout of this Window. */
    WindowLayoutPtr m_layout;

    /** The Window's render manager. */
    std::unique_ptr<RenderManager> m_render_manager;

    /** The GraphicsContext used to draw into this Window. */
    std::unique_ptr<GraphicsContext> m_graphics_context;

    /** Object drawing Cells into the Window. */
    std::unique_ptr<CellCanvas> m_cell_canvas;

    /** The Window's background color. */
    Color m_background_color;

    /** The Window size. */
    Size2i m_size;

    /** The first Item to receive mouse events.
     * When an Item handles a mouse press event, it will also receive -move and -release events, even if the cursor is
     * no longer within the Item.
     * May be empty.
     */
    std::weak_ptr<Widget> m_mouse_item;

    /** The first Item to receive keyboard events.
     * The 'focused' Item.
     * May be empty.
     */
    std::weak_ptr<Widget> m_keyboard_item;
};

} // namespace notf
