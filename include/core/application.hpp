#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include "common/handle.hpp"

struct GLFWwindow;

namespace signal {

class LayoutItem;
class LogHandler;
class ResourceManager;
class Window;

/// \brief The Application class.
///
/// Is a singleton, available everywhere with Application::instance();
/// Does not own any Windows (that is left to the client), but propagates events to all that are alive.
/// Manages the lifetime of the LogHandler.
class Application {

    friend class LayoutItem;
    friend class Window;
    friend bool register_item(std::shared_ptr<LayoutItem>);

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
    /// \return The application's return value.
    int exec();

    /// \brief Returns the next, free Handle.
    /// Is thread-safe.
    Handle get_next_handle()
    {
        Handle handle;
        do {
            handle = m_nextHandle++;
        } while (m_layout_items.count(handle));
        return handle;
    }

    /// \brief Returns a LayoutItem by its Handle.
    /// \param handle   Handle associated with the requested LayoutItem.
    std::shared_ptr<LayoutItem> get_item(Handle handle) const;

    /// \brief Returns the parent Handle of the given Handle.
    /// Returns ROOT_HANDLE if the given Handle is a root and BAD_HANDLE on failure.
    /// \param handle   Child Handle.
    Handle get_parent(Handle handle) const;

    /// \brief Returns the root Handle of the hierarchy containing the given Handle.
    /// The returned Handle may be the given handle itself, or BAD_HANDLE on failure.
    /// \param handle   Handle whose root to find.
    Handle get_root(Handle handle) const;

    /// \brief Returns the Application's Resource Manager.
    ResourceManager& get_resource_manager() { return *m_resource_manager; }

public: // static methods
    /// \brief The singleton Application instance.
    static Application& get_instance()
    {
        static Application instance;
        return instance;
    }

    /// \brief Called by GLFW in case of an error.
    /// \param error    Error ID.
    /// \param message  Error message;
    static void on_error(int error, const char* message);

    /// \brief Called by GLFW when a key is pressed, repeated or released.
    /// \param glfw_window  The GLFWwindow targeted by the event.
    /// \param key          Modified key.
    /// \param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// \param action       The action that triggered this callback.
    /// \param modifiers    Additional modifier key bitmask.
    static void on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /// \brief Called by GLFW, if the user requested a window to be closed.
    /// \param glfw_window  GLFW Window to close.
    static void on_window_close(GLFWwindow* glfw_window);

private: // methods for Window
    /// \brief Registers a new Window in this Application.
    void register_window(Window* window);

    /// \brief Unregisters an existing Window from this Application.
    void unregister_window(Window* window);

    /// \brief Changes the current Window of the Application.
    void set_current_window(Window* window);

private: // methods for LayoutItem
    /// \brief Defines a new parent Handle for the LayoutItem with the given child Handle.
    void set_parent(Handle child, Handle parent);

private: // methods
    /// \brief Constructor.
    explicit Application();

    /// \brief Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void shutdown();

    /// \brief Returns the Window instance associated with the given GLFW window.
    /// \param glfw_window  The GLFW window to look for.
    Window* get_window(GLFWwindow* glfw_window);

    /// \brief Removes handles to LayoutItems that have been deleted.
    /// It should rarely be necessary to call this function as the lookup complexity is constant on average.
    /// Can be useful to determine how many LayoutItems are currently alive though.
    void clean_unused_handles();

private: // structs
    /// \brief Helper struct exposing the LayoutItem's parent without having to lock the weak_ptr every time.
    struct LayoutItemData {
        Handle parent;
        std::weak_ptr<LayoutItem> layout_item;
    };

private: // fields
    /// \brief The next available handle, is ever-increasing.
    std::atomic<Handle> m_nextHandle;

    /// \brief The log handler thread used to format and print out log messages in a thread-safe manner.
    std::unique_ptr<LogHandler> m_log_handler;

    /// \brief The Application's resource manager.
    std::unique_ptr<ResourceManager> m_resource_manager;

    /// \brief All Windows known the the Application.
    std::unordered_map<GLFWwindow*, Window*> m_windows;

    /// \brief The Window with the current OpenGL context.
    Window* m_current_window;

    /// \brief All LayoutItems in the Application indexed by handle.
    mutable std::unordered_map<Handle, LayoutItemData> m_layout_items;
};

/// \brief Registers a new LayoutItem with the Application.
///
/// The handle of the LayoutItem may not be the BAD_HANDLE, nor may it have been used to register another LayoutItem.
///
/// \param item LayoutItem to register with its handle.
///
/// \return True iff the LayoutItem was successfully registered - false if its handle is already taken.
bool register_item(std::shared_ptr<LayoutItem> item);

} // namespace signal
