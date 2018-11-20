#pragma once

#include "notf/meta/log.hpp"

#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

/// The Application class.
/// The Application singleton is available everywhere with `TheApplication::get()`.
/// The first time you call `get()`, the Application is initialized with default arguments, unless you have already done
/// so yourself. Note that there are other objects that call `TheApplication::get` and might initialize the Application
/// implicitly (Window creation for example, requires TheApplication instance to exist). To make sure that your
/// initialization succeeds, call it in `main()` at your earliest convenience. Should you call `initialize` on an
/// already initialized Application, it will throw a `StartupError`, so you'll know when something went wrong.
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
    struct Args {

        // main arguments -----------------------------------------------------

        /// Command line arguments passed to main() by the OS.
        char** argv = nullptr;

        /// Number of strings in argv (first one is usually the name of the program).
        long argc = -1;

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

        // logger -------------------------------------------------------------

        /// Logging arguments.
        Logger::Args logger_arguments = {};
    };

private:
    /// Whether the Application has not been started, closed or if it is currently running.
    enum class State : size_t {
        UNSTARTED,
        RUNNING,
        CLOSED,
    };

    // methods --------------------------------------------------------------------------------- //
private:
    /// Constructor.
    /// @param args             Application arguments.
    /// @throws StartupError    When the Application intialization failed.
    TheApplication(Args args);

    /// Constructs default arguments on request.
    static const Args& _default_args()
    {
        static Args default_arguments;
        return default_arguments;
    }

    /// The Graph singleton.
    static TheApplication& _get_instance(const Args& application_args = _default_args())
    {
        if (s_state == State::CLOSED) { NOTF_THROW(ShutdownError, "The Application has already been shut down"); }
        s_state = State::RUNNING;

        static TheApplication instance(application_args);
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheApplication);

    /// Desctructor
    ~TheApplication();

    /// @{
    /// Initializes the Application through an user-defined ApplicationInfo object.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static TheApplication& initialize(const Args& application_args)
    {
        if (s_state != State::UNSTARTED) { NOTF_THROW(StartupError, "The Application has already been initialized"); }
        return _get_instance(application_args);
    }
    static TheApplication& initialize(const int argc, char* argv[])
    {
        Args args;
        args.argc = argc;
        args.argv = argv;
        return initialize(args);
    }
    /// @}

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    int exec();

    /// The singleton Application instance.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static TheApplication& get()
    {
        if (NOTF_LIKELY(is_running())) { return _get_instance(); }
        NOTF_THROW(ShutdownError, "You may not access the Application after it was shut down");
    }

    /// Application arguments as passed to the constructor.
    const Args& get_arguments() const { return m_args; }

    /// Checks if the Application was once open but is now closed.
    static bool is_running() { return s_state.load() == State::RUNNING; }

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

    /// The internal GLFW window managed by the Application
    /// Does not actually open a Window, only provides the shared OpenGL context.
    detail::GlfwWindowPtr m_shared_context;

    /// All Windows known the the Application.
    std::vector<WindowHandle> m_windows;

    /// Flag to indicate whether the Application is currently running.
    static inline std::atomic<State> s_state = State::UNSTARTED;
};

// accessors ======================================================================================================== //
template<>
class Accessor<TheApplication, Window> {
    friend class notf::Window;

    /// Registers a new Window in the Application.
    static void register_window(WindowHandle window) { TheApplication::get()._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    static void unregister_window(WindowHandle window) { TheApplication::get()._unregister_window(std::move(window)); }

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* get_shared_context() { return TheApplication::get().m_shared_context.get(); }
};

NOTF_CLOSE_NAMESPACE
