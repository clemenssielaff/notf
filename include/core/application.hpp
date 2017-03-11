#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/signal.hpp"

struct GLFWwindow;

namespace notf {

class LogHandler;
class ResourceManager;
class Window;

#ifdef NOTF_PYTHON
class PythonInterpreter;
#endif

/**********************************************************************************************************************/

/** Information for the Application.
 *
 * argv and arc
 * ============
 * To initialize the Application we require the `argv` and `argc` fields to be set and valid.
 *
 * Framerate
 * =========
 * The default values for `fps` and `enable_sync` will result in the most reliable 60fps refresh rate.
 * By setting the manual fps to something higher than 60, we let the hardware limit the framerate down to 60, which
 * gives a more stable 60fps than one that I can manually achieve by modifying the wait timeout in `Application::exec`.
 *
 * Folders
 * =======
 * The default ApplicationInfo contains default paths to resource folders, relative to the executable.
 */
struct ApplicationInfo {

    /** Command line arguments passed to main() by the OS. */
    char** argv = nullptr;

    /** Number of strings in argv (first one is usually the name of the program). */
    int argc = -1;

    /** Number of frames per seconds for animations.
     * Note that the number of redraws might be higher, as user interactions can trigger redraws at any time.
     * However, the Application::on_frame signal will fire around `fps` times per second.
     * Also see `enable_vsync`.
     */
    ushort fps = 100;

    /** If vertical synchronization is turned on or off.
     * If enabled, the effective application frame rate will be: min(fps, vsync-rate).
     * Usually the vsync-rate is 60 fps.
     */
    bool enable_vsync = true;

    bool _; // padding

    /** System path to the texture directory, absolute or relative to the executable. */
    std::string texture_directory = "res/textures/";

    /** System path to the fonts directory, absolute or relative to the executable. */
    std::string fonts_directory = "res/fonts/";

    /** System path to the application directory, absolute or relative to the executable. */
    std::string app_directory = "app/";
};

/**********************************************************************************************************************/

/** The Application class.
 * After initialization, the Application singleton is available everywhere with `Application::instance()`.
 * It also manages the lifetime of the LogHandler.
 */
class Application {

    friend class Window;

public: // enum
    /** Return codes of the Application's exec() function. */
    enum class RETURN_CODE : int {
        SUCCESS = 0,
        UNINITIALIZED,
        GLFW_FAILURE,
        PYTHON_FAILURE,
    };

public: // static methods
    /** Initializes the Application through an user-defined ApplicationInfo object. */
    static Application& initialize(const ApplicationInfo& info) { return _get_instance(info); }

    /** Initializes the Application using only the Command line arguments passed by the OS. */
    static Application& initialize(const int argc, char* argv[])
    {
        ApplicationInfo info;
        info.argc = argc;
        info.argv = argv;
        return _get_instance(info);
    }

    /** The singleton Application instance.
     * If you call this method before calling Application::initialize(), the program will exit with:
     *     RETURN_CODE::UNINITIALIZED
     */
    static Application& get_instance() { return _get_instance(); }

private: // constructor
    /** @param info     ApplicationInfo providing initialization arguments. */
    explicit Application(ApplicationInfo info);

public: // methods
    // no copy / assignment
    Application(const Application&) = delete;
    Application& operator=(Application) = delete;

    /** Desctructor */
    ~Application();

    /** Starts the application's main loop.
     * @return  The application's return value.
     */
    int exec();

    /** Returns the Application's Resource Manager. */
    ResourceManager& get_resource_manager() { return *m_resource_manager; }

    /** Returns the current Window. */
    std::shared_ptr<Window> get_current_window() { return m_current_window; }

    /** Returns the Application's Info. */
    const ApplicationInfo& get_info() const { return m_info; }

#ifdef NOTF_PYTHON
    /** Returns the Application's Python interpreter wrapper */
    /// Might be nullptr, if the Application was initialized with flag `enable_python` set to false.
    PythonInterpreter* get_python_interpreter() { return m_interpreter.get(); }
#endif

public: // signals
    /** Emitted info.fps times per seconds for Animations to update. */
    Signal<> on_frame;

private: // static methods
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

    /** Shuts down the application.
     * Is called automatically, after the last Window has been closed.
     */
    void _shutdown();

private: // fields
    /** The ApplicationInfo of this Application object. */
    ApplicationInfo m_info;

    /** The log handler thread used to format and print out log messages in a thread-safe manner. */
    std::unique_ptr<LogHandler> m_log_handler;

    /** The Application's resource manager. */
    std::unique_ptr<ResourceManager> m_resource_manager;

    /** All Windows known the the Application. */
    std::vector<std::shared_ptr<Window>> m_windows;

    /** The Window with the current OpenGL context. */
    std::shared_ptr<Window> m_current_window;

#ifdef NOTF_PYTHON
    /** The Python Interpreter embedded in the Application. */
    std::unique_ptr<PythonInterpreter> m_interpreter;
#endif
};

} // namespace notf
