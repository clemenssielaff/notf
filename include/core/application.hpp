#pragma once

#include <memory>
#include <unordered_map>

struct GLFWwindow;

namespace notf {

class ObjectManager;
class LogHandler;
class ResourceManager;
class Window;

/// \brief The Application class.
///
/// Is a singleton, available everywhere with Application::instance();
/// Does not own any Windows (that is left to the client), but propagates events to all that are alive.
/// It also manages the lifetime of the LogHandler.
class Application {

    friend class Window;

public:
    /// \brief Return codes of the Application's exec() function.
    enum class RETURN_CODE : int {
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

    /// \brief Returns the Application's Resource Manager.
    ResourceManager& get_resource_manager() { return *m_resource_manager; }

    /// \brief Returns the Application's Item Manager.
    ObjectManager& get_item_manager() { return *m_layout_item_manager; }

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

private: // methods
    /// \brief Constructor.
    explicit Application();

    /// \brief Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void shutdown();

    /// \brief Returns the Window instance associated with the given GLFW window.
    /// \param glfw_window  The GLFW window to look for.
    Window* get_window(GLFWwindow* glfw_window);

private: // fields
    /// \brief The log handler thread used to format and print out log messages in a thread-safe manner.
    std::unique_ptr<LogHandler> m_log_handler;

    /// \brief The Application's resource manager.
    std::unique_ptr<ResourceManager> m_resource_manager;

    /// \brief The Application's Item manger.
    std::unique_ptr<ObjectManager> m_layout_item_manager;

    /// \brief All Windows known the the Application.
    std::unordered_map<GLFWwindow*, Window*> m_windows;

    /// \brief The Window with the current OpenGL context.
    Window* m_current_window;
};

} // namespace notf
