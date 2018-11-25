#pragma once

#ifdef NOTF_TEST
#include "notf/meta/smart_factories.hpp"
#endif

#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// application ====================================================================================================== //

/// The Application class.
class Application {

    friend Accessor<Application, TheApplication>;
    friend Accessor<Application, Window>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Application);

    /// Exception thrown when the Application could not initialize.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(StartupError);

    /// Exception thrown when you try to access the Application instance after it was shut down.
    NOTF_EXCEPTION_TYPE(ShutdownError);

    /// Application arguments.
    struct Arguments {

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
    };

    // methods --------------------------------------------------------------------------------- //
private:
#ifdef NOTF_TEST
    NOTF_CREATE_SMART_FACTORIES(Application);
#endif

    /// Constructor.
    /// @param args             Application arguments.
    /// @throws StartupError    When the Application intialization failed.
    Application(Arguments args);

public:
    NOTF_NO_COPY_OR_ASSIGN(Application);

    /// Desctructor
    ~Application();

    /// Application arguments as passed to the constructor.
    const Arguments& get_arguments() const { return m_arguments; }

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    int exec() {
        int result;
        std::call_once(m_exec_once, [&] { result = _exec(); });
        return result;
    }

    /// Shuts down the application.
    /// Is called automatically, after the last Window has been closed, but also when the Application goes out of scope
    /// or this method is called by the user.
    void shutdown() {
        std::call_once(m_shutdown_once, [&] { _shutdown(); });
    }

private:
    /// The main loop.
    int _exec();

    /// Closes all Windows, cleans up all resources and gracefully shuts down the Application.
    void _shutdown();

    /// Registers a new Window in the Application.
    void _register_window(WindowHandle window);

    /// Unregisters an existing Window from this Application.
    void _unregister_window(WindowHandle window);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Application arguments as passed to the constructor.
    const Arguments m_arguments;

    /// The internal GLFW window managed by the Application
    /// Does not actually open a Window, only provides the shared OpenGL context.
    detail::GlfwWindowPtr m_shared_context;

    /// All Windows known the the Application.
    std::vector<WindowHandle> m_windows;

    /// Flag protecting the `exec` and `_shutdown` methods, which must only be called once.
    std::once_flag m_exec_once;
    std::once_flag m_shutdown_once;
};

// accessors ======================================================================================================== //

template<>
class Accessor<Application, TheApplication> {
    friend TheApplication;

#ifdef NOTF_TEST
    /// Creates a new Application instance wrapped in a unique_ptr.
    static auto create(Application::Arguments args) { return Application::_create_unique(args); }
#else
    /// Creates a new Application instance.
    static Application create(const Application::Arguments& args) { return Application(args); }
#endif
};

template<>
class Accessor<Application, Window> {
    friend Window;

    /// Registers a new Window in the Application.
    static void register_window(Application& app, WindowHandle window) { app._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    static void unregister_window(Application& app, WindowHandle window) { app._unregister_window(std::move(window)); }

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* get_shared_context(Application& app) { return app.m_shared_context.get(); }
};

// the application ================================================================================================== //

/// The Application singleton is available everywhere.
/// The first time you call `get()`, the Application is initialized with default arguments, unless you have already done
/// so yourself. Note that there are other objects that call `TheApplication::get` and might initialize the Application
/// implicitly (Window creation for example, requires an Application instance to exist). To make sure that your
/// initialization succeeds, call it in `main()` at your earliest convenience. Should you call `initialize` on an
/// already initialized Application, it will throw a `StartupError`, so you'll know when something went wrong.
class TheApplication {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheApplication);

    using StartupError = Application::StartupError;
    using ShutdownError = Application::ShutdownError;
    using Arguments = Application::Arguments;

private:
    /// Whether the Application singleton has not been started, closed or if it is currently running.
    enum class State : size_t {
        UNSTARTED,
        RUNNING,
        CLOSED,
    };

public:
    /// @{
    /// Initializes the Application singleton using user-defined arguments.
    /// @param args             Application arguments.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static Application& initialize(const Arguments& args) {
        if (s_state != State::UNSTARTED) { NOTF_THROW(StartupError, "The Application has already been initialized"); }
        return _get_instance(args);
    }
    static Application& initialize(const int argc, char* argv[]) {
        Arguments args;
        args.argc = argc;
        args.argv = argv;
        return initialize(args);
    }
    /// @}

    /// The singleton Application instance.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static Application& get() { return _get_instance(); }

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    static int exec() { return get().exec(); }

private:
    /// Constructs default arguments on request.
    static const Arguments& _default_args() {
        static Arguments default_arguments;
        return default_arguments;
    }

    /// The Graph singleton.
    /// @param args             Application arguments.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static Application& _get_instance(const Arguments& args = _default_args()) {

        if (State expected = State::UNSTARTED;
            !(s_state.compare_exchange_strong(expected, State::RUNNING)) && NOTF_UNLIKELY(expected == State::CLOSED)) {
            NOTF_THROW(ShutdownError, "The Application has already been shut down");
        }
        return _get_instance_internal(args);
    }

#ifdef NOTF_TEST
    /// For Testing, we need the ability to reinitialize the Application, something that would be quite dangerous to do
    /// in the wild. Therefore this option is only enabled for test builds.
    static Application& _get_instance_internal(const Arguments& args = _default_args(), const bool reinit = false) {
        static std::unique_ptr<Application> instance = Application::AccessFor<TheApplication>::create(args);
        if (NOTF_UNLIKELY(reinit)) { instance = Application::AccessFor<TheApplication>::create(args); }
        return *instance;
    }
#else
    static Application& _get_instance_internal(const Arguments& args = _default_args()) {
        static Application instance = Application::AccessFor<TheApplication>::create(args);
        return instance;
    }
#endif

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The state of the Application singleton.
    static inline std::atomic<State> s_state = State::UNSTARTED;
};

NOTF_CLOSE_NAMESPACE
