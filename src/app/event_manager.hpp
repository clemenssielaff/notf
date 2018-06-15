#pragma once

#include <condition_variable>
#include <deque>

#include "app/forwards.hpp"
#include "app/io/event.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/thread.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _EventManager;
} // namespace access

// ================================================================================================================== //

/// The EventManager is called from all parts of an Application, whenever an event happens.
/// Its job is to create Event objects that contain the type and auxiliary information about the event, and propagate
/// them to the relevant handlers.
///
/// The EventManager runs on the main thread, because it requires access to GLFW functions.
class EventManager {

    friend class access::_EventManager<Window>;
#ifdef NOTF_TEST
    friend class access::_EventManager<test::Harness>;
#endif

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_EventManager<T>;

private:
    /// Per-window data.
    class WindowHandler {

        // methods ------------------------------------------------------------
    public:
        /// Constructor.
        /// @param window   Window to forward the events into.
        explicit WindowHandler(Window* window) : m_window(window) {}

        /// Destructor.
        ~WindowHandler() { stop(); }

        /// Window to forward the events into.
        Window* window() { return m_window; }

        /// Start the handler.
        void start();

        /// Enqueue a new event for this handler.
        /// @param window   Window to redraw.
        void enqueue_event(EventPtr&& event);

        /// Stop the handler.
        /// Blocks until the worker thread joined.
        void stop();

    private:
        /// Worker method.
        void _run();

        // fields -------------------------------------------------------------
    private:
        /// Worker thread.
        ScopedThread m_thread;

        /// All events for this handler in order of occurrence.
        /// (Events in the front are older than the ones in the back).
        std::deque<EventPtr> m_events;

        /// Mutex guarding the RenderThread's state.
        Mutex m_mutex;

        /// Condition variable to wait for.
        ConditionVariable m_condition;

        /// Window to forward the events into.
        Window* const m_window;

        /// Is true as long at the thread should continue.
        bool m_is_running = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(EventManager);

    /// Constructor.
    EventManager();

    /// Destructor.
    /// Blocks until all current events have finished.
    ~EventManager();

    /// Handles a given event.
    /// @param event    Event to handle.
    void handle(EventPtr&& event);

    /// Suspends the handling of events.
    /// All events are stored but not handled until the manager resumes handling.
    void suspend();

    /// Resumes the handling of events.
    /// Executes all stored events in order.
    void resume();

private:
    /// Handles a given event.
    /// @param event    Event to handle.
    void _handle(EventPtr&& event);

    // window management ------------------------------------------------------

    /// Adds a new Window to the manager.
    /// @param window   Window to register.
    void _register_window(Window& window);

    /// Removes a Window from the manager - all remaining events for the Window are dropped immediately.
    /// @param window   Window to remove.
    void _remove_window(Window& window);

    // glfw event handler -----------------------------------------------------
private:
    /// Called by GLFW in case of an error.
    /// @param error    Error ID.
    /// @param message  Error message;
    static void _on_error(int error, const char* message);

    /// Called when the user presses or releases a mouse button Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param button       The mouse button triggering this callback.
    /// @param action       Mouse button action, is either GLFW_PRESS or GLFW_RELEASE.
    /// @param modifiers    Modifier key bitmask.
    static void _on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers);

    /// Called when the user moves the mouse inside a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            X coordinate of the cursor in Window coordinates.
    /// @param y            Y coordinate of the cursor in Window coordinates.
    static void _on_cursor_move(GLFWwindow* glfw_window, double x, double y);

    /// Called when the cursor enters or exists the client area of a Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param entered      Is GLFW_TRUE if the cursor entered the window's client area, or GLFW_FALSE if it left it.
    static void _on_cursor_entered(GLFWwindow* glfw_window, int entered);

    /// Called when the user scrolls inside the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            Horizontal scroll delta in screen coordinates.
    /// @param y            Vertical scroll delta in screen coordinates.
    static void _on_scroll(GLFWwindow* glfw_window, double x, double y);

    /// Called by GLFW when a key is pressed, repeated or released.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param key          Modified key.
    /// @param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// @param action       The action that triggered this callback.
    /// @param modifiers    Modifier key bitmask.
    static void _on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /// Called by GLFW when an unicode code point was generated.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void _on_char_input(GLFWwindow* glfw_window, uint codepoint);

    /// Called by GLFW when the user presses a key combination with at least one modifier key.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param codepoint    Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers    Modifier key bitmask.
    static void _on_shortcut(GLFWwindow* glfw_window, uint codepoint, int modifiers);

    /// Called when the Window was moved.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param x            The new x-coordinate of the top-left corner of the Window, in screen coordinates.
    /// @param y            The new y-coordinate of the top-left corner of the Window, in screen coordinates.
    static void _on_window_move(GLFWwindow* glfw_window, int x, int y);

    /// Called when the Window is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the Window.
    /// @param height       New height of the Window.
    static void _on_window_resize(GLFWwindow* glfw_window, int width, int height);

    /// Called when the Window's framebuffer is resized.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param width        New width of the framebuffer.
    /// @param height       New height of the framebuffer.
    static void _on_framebuffer_resize(GLFWwindow* glfw_window, int width, int height);

    /// Called when the Window is refreshed by the OS.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    static void _on_window_refresh(GLFWwindow* glfw_window);

    /// Called when the Window has gained/lost OS focus.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE if the window gained focus, or GLFW_FALSE if it lost it.
    static void _on_window_focus(GLFWwindow* glfw_window, int kind);

    /// Called when the Window is minimzed.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param kind         Is GLFW_TRUE when the Window was minimized, or GLFW_FALSE if it was restored.
    static void _on_window_minimize(GLFWwindow* glfw_window, int kind);

    /// Called by GLFW when the user drops one or more files into the Window.
    /// @param glfw_window  The GLFWwindow targeted by the event
    /// @param count        Number of dropped files.
    /// @param paths        The UTF-8 encoded file and/or directory path names.
    static void _on_file_drop(GLFWwindow* glfw_window, int count, const char** paths);

    /// Called by GLFW, if the user requested a window to be closed.
    /// @param glfw_window  GLFW Window to close.
    static void _on_window_close(GLFWwindow* glfw_window);

    /// Called by GLFW, if the user connects or disconnects a monitor.
    /// @param glfw_monitor The monitor that was connected or disconnected.
    /// @param kind         Either GLFW_CONNECTED or GLFW_DISCONNECTED.
    static void _on_monitor_change(GLFWmonitor* glfw_monitor, int kind);

    /// Called when a joystick was connected or disconnected.
    /// @param joystick The joystick that was connected or disconnected.
    /// @param kind     One of GLFW_CONNECTED or GLFW_DISCONNECTED.
    static void _on_joystick_change(int joystick, int kind);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Relevant data for each Window.
    /// Handlers are unordered.
    std::vector<std::unique_ptr<WindowHandler>> m_handler;

    /// When the event handler is suspended, all events are queued into the backlog instead.
    std::vector<EventPtr> m_backlog;

    /// Mutex making sure that event dispatch is thread-safe.
    Mutex m_mutex;

    /// When suspended, all events are stored but not handled until the manager resumes handling.
    bool m_is_suspended = false;
};

// ================================================================================================================== //

template<>
class access::_EventManager<Window> {
    friend class notf::Window;

    /// Adds a new Window to the manager.
    /// @param event_manager    EventManager to operate on.
    /// @param window           Window to register.
    static void register_window(EventManager& event_manager, Window& window) { event_manager._register_window(window); }

    /// Removes a Window from the manager - all remaining events for the Window are dropped immediately.
    /// @param event_manager    EventManager to operate on.
    /// @param window           Window to remove.
    static void remove_window(EventManager& event_manager, Window& window) { event_manager._remove_window(window); }
};

NOTF_CLOSE_NAMESPACE
