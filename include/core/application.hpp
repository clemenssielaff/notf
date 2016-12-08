#pragma once

#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace notf {

class LogHandler;
class PythonInterpreter;
class ResourceManager;
class Window;

/**********************************************************************************************************************/

/// @brief Information for the Application.
/// To initialize the Application we require the `argv` and `argc` fields to be set and valid.
struct ApplicationInfo {

    /// @brief Command line arguments passed to main() by the OS.
    char** argv = nullptr;

    /// @brief Number of strings in argv (first one is usually the name of the program).
    int argc = -1;

    /** If set to false, the Application will not have a Python interpreter available. */
    bool enable_python = true;

    /** System path to the texture directory, absolute or relative to the executable. */
    std::string texture_directory = "res/textures/";

    /** System path to the fonts directory, absolute or relative to the executable. */
    std::string fonts_directory = "res/fonts/";

    /** System path to the application directory, absolute or relative to the executable. */
    std::string app_directory = "app/";
};

/**********************************************************************************************************************/

/// @brief The Application class.
///
/// Is a singleton, available everywhere with Application::instance();
/// Does not own any Windows (that is left to the client), but propagates events to all that are alive.
/// It also manages the lifetime of the LogHandler.
class Application {

    friend class Window;

public:
    /// @brief Return codes of the Application's exec() function.
    enum class RETURN_CODE : int {
        SUCCESS = 0,
        UNINITIALIZED,
        GLFW_FAILURE,
        PYTHON_FAILURE,
        NANOVG_FAILURE,
    };

public: // methods
    /// @brief Copy constructor.
    Application(const Application&) = delete;

    /// @brief Assignment operator.
    Application& operator=(Application) = delete;

    /// @brief Destructor.
    ~Application();

    /// @brief Starts the application's main loop.
    /// @return The application's return value.
    int exec();

    /// @brief Returns the Application's Resource Manager.
    ResourceManager& get_resource_manager() { return *m_resource_manager; }

    /// @brief Returns the Application's Python interpreter wrapper
    /// Might be nullptr, if the Application was initialized with flag `enable_python` set to false.
    PythonInterpreter* get_python_interpreter() { return m_interpreter.get(); }

    /// @brief Returns the current Window.
    std::shared_ptr<Window> get_current_window() { return m_current_window; }

    /// @brief Returns the Application's Info.
    const ApplicationInfo& get_info() const { return m_info; }

public: // static methods
    /** Initializes the Application through an user-defined ApplicationInfo object.
     * Any relative path in the info (including the default paths) will be resolved after this method has been called,
     * meaning for example, that you can use `info.app_directory` to generate absolute paths to python scripts.
     */
    static Application& initialize(ApplicationInfo &info);

    /// @brief Initializes the Application using only the Command line arguments passed by the OS.
    static Application& initialize(const int argc, char* argv[])
    {
        ApplicationInfo info;
        info.argc = argc;
        info.argv = argv;
        return _get_instance(std::move(info));
    }

    /// @brief The singleton Application instance.
    static Application& get_instance() { return _get_instance(); }

    /// @brief Called by GLFW in case of an error.
    /// @param error    Error ID.
    /// @param message  Error message;
    static void _on_error(int error, const char* message);

    /// @brief Called by GLFW when a key is pressed, repeated or released.
    /// @param glfw_window  The GLFWwindow targeted by the event.
    /// @param key          Modified key.
    /// @param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// @param action       The action that triggered this callback.
    /// @param modifiers    Additional modifier key bitmask.
    static void _on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /// @brief Called by GLFW, if the user requested a window to be closed.
    /// @param glfw_window  GLFW Window to close.
    static void _on_window_close(GLFWwindow* glfw_window);

    /// @brief Called when the Window is resize.
    /// @param glfw_window  Resized windwow.
    /// @param width        New width of the Window.
    /// @param height       New height of the Window.
    static void _on_window_reize(GLFWwindow* glfw_window, int width, int height);

private: // methods for Window
    /// @brief Registers a new Window in this Application.
    void _register_window(std::shared_ptr<Window> window);

    /// @brief Unregisters an existing Window from this Application.
    void _unregister_window(std::shared_ptr<Window> window);

    /// @brief Changes the current Window of the Application.
    void _set_current_window(Window* window);

private: // methods
    /// @brief Static (private) function holding the actual Application instance.
    static Application& _get_instance(const ApplicationInfo& info = ApplicationInfo())
    {
        static Application instance(info);
        return instance;
    }

    /// @brief Constructor.
    /// @param info     ApplicationInfo providing initialization arguments.
    explicit Application(const ApplicationInfo info);

    /// @brief Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void _shutdown();

private: // fields
    /// @brief The ApplicationInfo of this Application object.
    const ApplicationInfo m_info;

    /// @brief The log handler thread used to format and print out log messages in a thread-safe manner.
    std::unique_ptr<LogHandler> m_log_handler;

    /// @brief The Application's resource manager.
    std::unique_ptr<ResourceManager> m_resource_manager;

    /// @brief All Windows known the the Application.
    std::vector<std::shared_ptr<Window>> m_windows;

    /// @brief The Python Interpreter embedded in the Application.
    std::unique_ptr<PythonInterpreter> m_interpreter;

    /// @brief The Window with the current OpenGL context.
    std::shared_ptr<Window> m_current_window;
};

} // namespace notf

// TODO: Application::on_tick signal
