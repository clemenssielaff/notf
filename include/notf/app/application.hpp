#pragma once

#include "notf/meta/log.hpp"

#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

/// The Application class.
/// After initialization, the Application singleton is available everywhere with `TheApplication::get()`.
class TheApplication {

    friend Accessor<TheApplication, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheApplication);

    /// Exception thrown when the Application could not initialize.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(StartupError);

    /// Exception thrown when you try to access the Application instance after it was shut down.
    NOTF_EXCEPTION_TYPE(ShutdownError);

    /// Application arguments.
    ///
    /// argv and arc
    /// ============
    /// To initialize the Application we require the `argv` and `argc` fields to be set and valid.
    ///
    /// Directories
    /// =======
    /// The default ApplicationInfo contains default paths to resource folders, relative to the executable.
    struct Args {

        // main arguments -----------------------------------------------------

        /// Command line arguments passed to main() by the OS.
        char** argv = nullptr;

        /// Number of strings in argv (first one is usually the name of the program).
        int argc = -1;

        // logger -------------------------------------------------------------

        /// Logging arguments.
        Logger::Args logger_arguments = {};

        // directories --------------------------------------------------------

        /// Base directory to load resources from.
        std::string resource_directory = "res/";

        /// System path to the texture directory, relative to the resource directory.
        std::string texture_directory = "textures/";

        /// System path to the fonts directory, relative to the resource directory.
        std::string fonts_directory = "fonts/";

        /// System path to the shader directory, relative to the resource directory.
        std::string shader_directory = "shaders/";

        /// System path to the application directory, absolute or relative to the executable.
        std::string app_directory = "app/";
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Constructor.
    /// @param application_args         Application arguments.
    /// @throws initialization_error    When the Application intialization failed.
    TheApplication(Args args);

    /// Creates invalid Application arguments that trigger an exception in the constructor.
    /// Needed for when the user forgets to call "initialize" with valid arguments.
    static const Args& _default_args()
    {
        static Args default_arguments;
        return default_arguments;
    }

    /// The Graph singleton.
    static TheApplication& _get(const Args& application_args = _default_args())
    {
        static TheApplication instance(application_args);
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheApplication);

    /// Desctructor
    ~TheApplication();

    /// Initializes the Application through an user-defined ApplicationInfo object.
    /// @throws initialization_error    When the Application intialization failed.
    static TheApplication& initialize(const Args& application_args) { return _get(application_args); }

    /// Initializes the Application using only the command line arguments passed by the OS.
    /// @throws initialization_error    When the Application intialization failed.
    static TheApplication& initialize(const int argc, char* argv[])
    {
        Args args;
        args.argc = argc;
        args.argv = argv;
        return initialize(args);
    }

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    int exec();

    /// The singleton Application instance.
    /// @throws initialization_error    When the Application intialization failed.
    /// @throws shut_down_error         When this method is called after the Application was shut down.
    static TheApplication& get()
    {
        if (NOTF_LIKELY(is_running())) { return _get(); }
        NOTF_THROW(ShutdownError, "You may not access the Application after it was shut down");
    }

    /// Application arguments as passed to the constructor.
    const Args& get_arguments() const { return m_args; }

    /// Checks if the Application was once open but is now closed.
    static bool is_running() { return s_is_running.load(); }

private:
    /// Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void _shutdown();

    /// Registers a new Window in the Application.
    void _register_window(WindowHandle window);

    /// Unregisters an existing Window from this Application.
    void _unregister_window(WindowHandle window);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Application arguments as passed to the constructor.
    const Args m_args;

    /// The internal GLFW window managed by the Application.
    /// Does not actually open a Window, only provides the shared OpenGL context.
    detail::GlfwWindowPtr m_shared_window;

    /// All Windows known the the Application.
    std::vector<WindowHandle> m_windows;

    /// Flag to indicate whether the Application is currently running or not.
    inline static std::atomic<bool> s_is_running{true};
};

// accessors ======================================================================================================== //
template<>
class Accessor<TheApplication, Window> {
    friend class notf::Window;

    /// Registers a new Window in the Application.
    static void register_window(WindowHandle window) { TheApplication::get()._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    static void unregister_window(WindowHandle window) { TheApplication::get()._unregister_window(std::move(window)); }

    /// The internal GLFW window managed by the Application.
    static GLFWwindow* get_shared_window() { return TheApplication::get().m_shared_window.get(); }
};

NOTF_CLOSE_NAMESPACE
