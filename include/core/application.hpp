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

/** Information for the Application.
 * To initialize the Application we require the `argv` and `argc` fields to be set and valid.
 */
struct ApplicationInfo {

    /** Command line arguments passed to main() by the OS. */
    char** argv = nullptr;

    /** Number of strings in argv (first one is usually the name of the program). */
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

/** The Application class.
 * Is a singleton, available everywhere with Application::instance();
 * Does not own any Windows (that is left to the client), but propagates events to all that are alive.
 * It also manages the lifetime of the LogHandler.
 */
class Application {

    friend class Window;

public:
    /** Return codes of the Application's exec() function. */
    enum class RETURN_CODE : int {
        SUCCESS = 0,
        UNINITIALIZED,
        GLFW_FAILURE,
        PYTHON_FAILURE,
    };

public: // methods
    // no copy / assignment
    Application(const Application&) = delete;
    Application& operator=(Application) = delete;

    ~Application();

    /** Starts the application's main loop.
     * @return  The application's return value.
     */
    int exec();

    /** Returns the Application's Resource Manager. */
    ResourceManager& get_resource_manager() { return *m_resource_manager; }

    /** Returns the Application's Python interpreter wrapper */
    /// Might be nullptr, if the Application was initialized with flag `enable_python` set to false.
    PythonInterpreter* get_python_interpreter() { return m_interpreter.get(); }

    /** Returns the current Window. */
    std::shared_ptr<Window> get_current_window() { return m_current_window; }

    /** Returns the Application's Info. */
    const ApplicationInfo& get_info() const { return m_info; }

public: // static methods
    /** Initializes the Application through an user-defined ApplicationInfo object.
     * Any relative path in the info (including the default paths) will be resolved after this method has been called,
     * meaning for example, that you can use `info.app_directory` to generate absolute paths to python scripts.
     */
    static Application& initialize(ApplicationInfo& info);

    /** Initializes the Application using only the Command line arguments passed by the OS. */
    static Application& initialize(const int argc, char* argv[])
    {
        ApplicationInfo info;
        info.argc = argc;
        info.argv = argv;
        return _get_instance(std::move(info));
    }

    /** The singleton Application instance. */
    static Application& get_instance() { return _get_instance(); }

    /** Called by GLFW in case of an error.
     * @param error    Error ID.
     * @param message  Error message;
     */
    static void _on_error(int error, const char* message);

    /** Called by GLFW when a key is pressed, repeated or released.
     * @param glfw_window   The GLFWwindow targeted by the event.
     * @param key           Modified key.
     * @param scancode      May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
     * @param action        The action that triggered this callback.
     * @param modifiers     Modifier key bitmask.
     */
    static void _on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /** Called when the user moves the mouse inside a Window.
     * @param glfw_window   The GLFWwindow targeted by the event.
     * @param x             X coordinate of the cursor in Window coordinates.
     * @param y             Y coordinate of the cursor in Window coordinates.
     */
    static void _on_cursor_move(GLFWwindow* glfw_window, double x, double y);

    /** Called when the user presses or releases a mouse button Window.
     * @param glfw_window   The GLFWwindow targeted by the event.
     * @param button        The mouse button triggering this callback.
     * @param action        Mouse button action, is either PRESS or RELEASE.
     * @param modifiers     Modifier key bitmask.
     */
    static void _on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers);

    /** Called by GLFW, if the user requested a window to be closed.
     * @param glfw_window  GLFW Window to close.
     */
    static void _on_window_close(GLFWwindow* glfw_window);

    /** Called when the Window is resize.
     * @param glfw_window   Resized windwow.
     * @param width         New width of the Window.
     * @param height        New height of the Window.
     */
    static void _on_window_resize(GLFWwindow* glfw_window, int width, int height);

private: // methods for Window
    /** Registers a new Window in this Application. */
    void _register_window(std::shared_ptr<Window> window);

    /** Unregisters an existing Window from this Application. */
    void _unregister_window(std::shared_ptr<Window> window);

    /** Changes the current Window of the Application. */
    void _set_current_window(Window* window);

private: // methods
    /** Static (private) function holding the actual Application instance. */
    static Application& _get_instance(const ApplicationInfo& info = ApplicationInfo())
    {
        static Application instance(info);
        return instance;
    }

    /** param info     ApplicationInfo providing initialization arguments. */
    explicit Application(const ApplicationInfo info);

    /** Shuts down the application.
     * Is called automatically, after the last Window has been closed.
     */
    void _shutdown();

private: // fields
    /** The ApplicationInfo of this Application object. */
    const ApplicationInfo m_info;

    /** The log handler thread used to format and print out log messages in a thread-safe manner. */
    std::unique_ptr<LogHandler> m_log_handler;

    /** The Application's resource manager. */
    std::unique_ptr<ResourceManager> m_resource_manager;

    /** All Windows known the the Application. */
    std::vector<std::shared_ptr<Window>> m_windows;

    /** The Python Interpreter embedded in the Application. */
    std::unique_ptr<PythonInterpreter> m_interpreter;

    /** The Window with the current OpenGL context. */
    std::shared_ptr<Window> m_current_window;
};

} // namespace notf

// TODO: Application::on_tick signal
