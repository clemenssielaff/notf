#pragma once

#include "app/forwards.hpp"
#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Exception thrown when the Application could not initialize.
/// The error string will contain more detailed information about the error.
NOTF_EXCEPTION_TYPE(application_initialization_error)

//====================================================================================================================//

/// The Application class.
/// After initialization, the Application singleton is available everywhere with `Application::instance()`.
/// It also manages the lifetime of the LogHandler.
class Application {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Window)

    /// Application arguments.
    ///
    /// argv and arc
    /// ============
    /// To initialize the Application we require the `argv` and `argc` fields to be set and valid.
    ///
    /// Framerate
    /// =========
    /// The default values for `fps` and `enable_sync` will result in the most reliable 60fps refresh rate.
    /// By setting the manual fps to something higher than 60, we let the hardware limit the framerate down to 60, which
    /// gives a more stable 60fps than one that I can manually achieve by modifying the wait timeout in
    /// `Application::exec`.
    ///
    /// Folders
    /// =======
    /// The default ApplicationInfo contains default paths to resource folders, relative to the executable.
    struct Args {

        /// System path to the texture directory, can either be absolute or relative to the executable.
        std::string texture_directory = "res/textures/";

        /// System path to the fonts directory, absolute or relative to the executable.
        std::string fonts_directory = "res/fonts/";

        /// System path to the shader directory, absolute or relative to the executable.
        std::string shader_directory = "res/shaders/";

        /// System path to the application directory, absolute or relative to the executable.
        std::string app_directory = "app/";

        /// Command line arguments passed to main() by the OS.
        char** argv = nullptr;

        /// Number of strings in argv (first one is usually the name of the program).
        int argc = -1;

        uint max_fps = 0;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Constructor.
    /// @param application_args                     Application arguments.
    /// @throws application_initialization_error    When the Application intialization failed.
    Application(const Args& application_args);

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Application)
    NOTF_NO_HEAP_ALLOCATION(Application)

    /// Desctructor
    ~Application();

    // initialization  --------------------------------------------------------

    /// Initializes the Application through an user-defined ApplicationInfo object.
    /// @throws application_initialization_error    When the Application intialization failed.
    static Application& initialize(const Args& application_args) { return _instance(application_args); }

    /// Initializes the Application using only the command line arguments passed by the OS.
    /// @throws application_initialization_error    When the Application intialization failed.
    static Application& initialize(const int argc, char* argv[])
    {
        Args args;
        args.argc = argc;
        args.argv = argv;
        return _instance(std::move(args));
    }

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    int exec();

    // global state -----------------------------------------------------------

    /// The singleton Application instance.
    /// @returns    The Application singleton.
    /// @throws application_initialization_error    When the Application intialization failed.
    static Application& instance() { return _instance(); }

    /// The Application's Resource Manager.
    ResourceManager& resource_manager() { return *m_resource_manager; }

    /// The Application's thread pool.
    ThreadPool& thread_pool() { return *m_thread_pool; }

    /// The Application's PropertyManager.
    PropertyManager& property_graph() { return *m_property_graph; }

private:
    /// Static (private) function holding the actual Application instance.
    static Application& _instance(const Args& application_args = s_invalid_args)
    {
        static Application instance(application_args);
        return instance;
    }

    /// Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void _shutdown();

    // window -----------------------------------------------------------------

    /// Registers a new Window in this Application.
    /// @throws window_initialization_error     If the Window has no valid associated OpenGL context.
    void _register_window(const WindowPtr& window);

    /// Unregisters an existing Window from this Application.
    void _unregister_window(Window* window);

    /// Changes the current Window of the Application.
    void _set_current_window(Window* window);

    // event handler ----------------------------------------------------------

    /// Called by GLFW in case of an error.
    /// @param error    Error ID.
    /// @param message  Error message;
    static void _on_error(int error, const char* message);

    /// Called by GLFW when a key is pressed, repeated or released.
    /// @param glfw_window   The GLFWwindow targeted by the event.
    /// @param key           Modified key.
    /// @param scancode      May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// @param action        The action that triggered this callback.
    /// @param modifiers     Modifier key bitmask.
    static void _on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int modifiers);

    /// Called by GLFW when an unicode code point was generated.
    /// @param glfw_window   The GLFWwindow targeted by the event
    /// @param codepoint     Unicode code point generated by the event as native endian UTF-32.
    /// @param modifiers     Modifier key bitmask.
    static void _on_char_input(GLFWwindow* glfw_window, uint codepoint, int modifiers);

    /// Called when the cursor enters or exists the client area of a Window.
    /// @param glfw_window   The GLFWwindow targeted by the event
    /// @param entered       0 => cursor left | !0 => cursor entered
    static void _on_cursor_entered(GLFWwindow* glfw_window, int entered);

    /// Called when the user moves the mouse inside a Window.
    /// @param glfw_window   The GLFWwindow targeted by the event.
    /// @param x             X coordinate of the cursor in Window coordinates.
    /// @param y             Y coordinate of the cursor in Window coordinates.
    static void _on_cursor_move(GLFWwindow* glfw_window, double x, double y);

    /// Called when the user presses or releases a mouse button Window.
    /// @param glfw_window   The GLFWwindow targeted by the event.
    /// @param button        The mouse button triggering this callback.
    /// @param action        Mouse button action, is either PRESS or RELEASE.
    /// @param modifiers     Modifier key bitmask.
    static void _on_mouse_button(GLFWwindow* glfw_window, int button, int action, int modifiers);

    /// Called when the user scrolls inside the Window.
    /// @param glfw_window   The GLFWwindow targeted by the event.
    /// @param x             Horizontal scroll delta in screen coordinates.
    /// @param y             Vertical scroll delta in screen coordinates.
    static void _on_scroll(GLFWwindow* glfw_window, double x, double y);

    /// Called by GLFW, if the user requested a window to be closed.
    /// @param glfw_window  GLFW Window to close.
    static void _on_window_close(GLFWwindow* glfw_window);

    /// Called when the Window is resize.
    /// @param glfw_window   Resized windwow.
    /// @param width         New width of the Window.
    /// @param height        New height of the Window.
    static void _on_window_resize(GLFWwindow* glfw_window, int width, int height);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// The LogHandler thread used to format and print out log messages in a thread-safe manner.
    LogHandlerPtr m_log_handler;

    /// The Application's resource manager.
    ResourceManagerPtr m_resource_manager;

    /// The global thread pool.
    ThreadPoolPtr m_thread_pool;

    /// PropertyManager.
    PropertyManagerPtr m_property_graph;

    /// All Windows known the the Application.
    std::vector<WindowPtr> m_windows;

    /// Invalid arguments, used when you call `get_instance` without initializing the Application first.
    static const Args s_invalid_args;
};

// ===================================================================================================================//

template<>
class Application::Access<Window> {
    friend class Window;

    /// Constructor.
    Access() : m_application(Application::instance()) {}

    /// Registers a new Window in this Application.
    void register_new(WindowPtr window) { m_application._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    void unregister(Window* window) { m_application._unregister_window(std::move(window)); }

    /// The Application to access.
    Application& m_application;
};

NOTF_CLOSE_NAMESPACE
