#pragma once

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// the application ================================================================================================== //

/// The Application singleton is available everywhere.
/// The first time you call `get()`, the Application is READY with default arguments, unless you have already done
/// so yourself. Note that there are other objects that call `TheApplication::get` and might initialize the Application
/// implicitly (Window creation for example, requires an Application instance to exist). To make sure that your
/// initialization succeeds, call it in `main()` at your earliest convenience. Should you call `initialize` on an
/// already READY Application, it will throw a `StartupError`, so you'll know when something went wrong.
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

private:
    /// Whether the Application singleton has not been started, closed or if it is currently running.
    enum class State : size_t {
        EMPTY,
        READY,
        RUNNING,
        SHUTDOWN,
    };

    // application ============================================================

    /// The Application class.
    class Application {

        friend class TheApplication;

        // methods --------------------------------------------------------------------------------- //
    public:
        NOTF_NO_COPY_OR_ASSIGN(Application);

        /// Constructor.
        /// @param args             Application arguments.
        /// @throws StartupError    When the Application intialization failed.
        Application(Arguments args);

        /// Desctructor
        ~Application();

        /// Starts the application's main loop.
        /// @return  The application's return value after it has finished.
        int exec();

        /// Shuts down the Application at the next possibility.
        void shutdown();

        /// Registers a new Window in the Application.
        void register_window(WindowHandle window);

        /// Unregisters an existing Window from this Application.
        void unregister_window(WindowHandle window);

    private:
        /// Actual shutdown procedure, is only exexuted once.
        void _shutdown();

        // fields ---------------------------------------------------------------------------------- //
    private:
        /// Application arguments as passed to the constructor.
        const Arguments m_arguments;

        /// The internal GLFW window managed by the Application
        /// Does not actually open a Window, only provides the shared OpenGL context.
        detail::GlfwWindowPtr m_shared_context;

        /// All Windows known the the Application.
        std::unique_ptr<std::vector<WindowHandle>> m_windows;

        /// Application-wide event scheduler.
        SchedulerPtr m_scheduler;

        /// State of the Application.
        /// EMPTY -> READY -> RUNNING -> SHUTDOWN -> EMPTY
        std::atomic<State> m_state = State::EMPTY;
    };

public:
    /// @{
    /// Initializes the Application singleton using user-defined arguments.
    /// @param args             Application arguments.
    /// @throws StartupError    When the Application intialization failed.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static void initialize(const Arguments& args) {
        if (State expected = State::EMPTY; s_state.compare_exchange_strong(expected, State::READY)) {
            _get(args);
        } else {
            NOTF_THROW(StartupError, "The Application has already been initialized");
        }
    }
    static void initialize(const int argc, char* argv[]) {
        Arguments args;
        args.argc = argc;
        args.argv = argv;
        initialize(args);
    }
    /// @}

    /// Application arguments as passed to the constructor.
    static const Arguments& get_arguments() { return _get().m_arguments; }

    /// Application-wide Scheduler.
    static Scheduler& get_scheduler() { return *_get().m_scheduler.get(); }

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    /// @throws StartupError    When the Application is already running.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    static int exec() {
        int result = _get().exec();
        s_state.store(State::SHUTDOWN);
        return result;
    }

    /// Forces a shutdown of a running Application.
    /// If the Application is not running, this does nothing.
    static void shutdown() {
        if (s_state == State::READY) { // do not initialize if empty or re-initialize if already shut down
            _get().shutdown();
            s_state.store(State::SHUTDOWN);
        }
    }

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
    static Application& _get(const Arguments& args = _default_args()) {
        if (State actual = State::EMPTY;
            !(s_state.compare_exchange_strong(actual, State::READY)) && NOTF_UNLIKELY(actual == State::SHUTDOWN)) {
            NOTF_THROW(ShutdownError, "The Application has already been shut down");
        }
#ifndef NOTF_TEST
        static Application instance(args);
        return instance;
#else
        /// in test builds, it is possible to force-reinitialize the Application
        static auto instance = std::make_unique<Application>(args);
        if (s_reinit) {
            s_reinit = false;
            instance = std::make_unique<Application>(args);
            s_state.store(State::READY);
        }
        return *instance;
#endif
    }

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* _get_shared_context() { return _get().m_shared_context.get(); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The state of the Application singleton.
    /// EMPTY -> READY -> SHUTDOWN
    static inline std::atomic<State> s_state = State::EMPTY;

#ifdef NOTF_TEST
    /// Is true, if the Application should re-initialize the next time its instance is requested.
    static inline bool s_reinit = false;
#endif
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheApplication, Window> {
    friend Window;

    /// Registers a new Window in the Application.
    static void register_window(WindowHandle window);

    /// Unregisters an existing Window from this Application.
    static void unregister_window(WindowHandle window);

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* get_shared_context() { return TheApplication::_get_shared_context(); }
};

NOTF_CLOSE_NAMESPACE
