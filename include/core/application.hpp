#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <unordered_map>

#include "core/component.hpp"
#include "core/keyboard.hpp"
#include "common/handle.hpp"
#include "common/log.hpp"

struct GLFWwindow;

namespace signal {

class Widget;
class Window;

/// \brief The Application class.
///
/// Is a singleton, available everywhere with Application::instance();
/// Does not own any Windows (that is left to the client), but propagates events to all registered.
/// Manages the lifetime of the LogHandler.
class Application {

    friend class Component;
    friend class Widget;
    friend class Window;

public:
    /// \brief Return codes of the Application's exec() function.
    enum class RETURN_CODE {
        SUCCESS = 0,
        FAILURE = 1,
    };

public: // methods
    /// \brief Copy constructor.
    Application(const Application&) = delete;

    /// \brief Assignment operator.
    Application& operator=(Application) = delete;

    /// \brief Destructor.
    ~Application();

    /// \brief Starts the application's main loop.
    ///
    /// \return The application's return value.
    int exec();

    /// \brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle get_next_handle()
    {
        Handle handle;
        do {
            handle = m_nextHandle++;
        } while (m_widgets.count(handle));
        return handle;
    }

    /// \brief Returns a Widget by its Handle.
    ///
    /// \param handle   Handle associated with the requested Widget.
    std::shared_ptr<Widget> get_widget(Handle handle);

public: // static methods
    /// \brief The singleton Application instance.
    static Application& get_instance()
    {
        static Application instance;
        return instance;
    }

    /// \brief Called by GLFW in case of an error.
    ///
    /// \param error    Error ID.
    /// \param message  Error message;
    static void on_error(int error, const char* message);

    /// \brief Called by GLFW when a key is pressed, repeated or released.
    ///
    /// \param glfw_window  The GLFWwindow targeted by the event.
    /// \param key          Modified key.
    /// \param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// \param action       The action that triggered this callback.
    /// \param modifiers    Additional modifier key bitmask.
    static void on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /// \brief Called by GLFW, if the user requested a window to be closed.
    ///
    /// \param glfw_window  GLFW Window to close.
    static void on_window_close(GLFWwindow* glfw_window);

private: // methods for Component
    /// \brief Registers a dirty Component - it will be updated before the next frame.
    ///
    /// \param component    Component to update.
    void register_dirty_component(std::shared_ptr<Component> component);

private: // methods for Widget
    /// \brief Registers a new Widget with the Application.
    ///
    /// The handle of the Widget may not be the BAD_HANDLE, nor may it have been used to register another Widget.
    ///
    /// \param widget   Widget to register with its handle.
    ///
    /// \return True iff the Widget was successfully registered - false if its handle is already taken.
    bool register_widget(std::shared_ptr<Widget> widget);

private: // methods for Window
    /// \brief Registers a new Window in this Application.
    void register_window(Window* window);

    /// \brief Unregisters an existing Window from this Application.
    void unregister_window(Window* window);

private: // methods
    /// \brief Constructor.
    explicit Application();

    /// \brief Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void shutdown();

    /// \brief Returns the Window instance associated with the given GLFW window.
    ///
    /// \param glfw_window  The GLFW window to look for.
    Window* get_window(GLFWwindow* glfw_window);

    /// \brief Removes handles to Widgets that have been deleted.
    ///
    /// It should rarely be necessary to call this function as the lookup complexity is constant on average.
    /// Can be useful to determine how many Widgets are currently alive though.
    void clean_unused_handles();

private: // fields
    /// \brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// \brief The log handler thread used to format and print out log messages in a thread-safe manner.
    LogHandler m_log_handler;

    /// \brief The current state of all keyboard keys.
    KeyStateSet m_key_states;

    /// \brief A list of lists of dirty Components that need to be updated for the next frame.
    /// The index of the Component vector in the array corresponds to the Component's kind.
    /// Components are updated in ascending order of their kind.
    std::array<std::vector<std::shared_ptr<Component> >, Component::get_count()> m_dirty_components;

    /// \brief All Windows known the the Application.
    std::unordered_map<GLFWwindow*, Window*> m_windows;

    /// \brief All Widgets in the Application indexed by handle.
    std::unordered_map<Handle, std::weak_ptr<Widget> > m_widgets;
};

} // namespace signal
