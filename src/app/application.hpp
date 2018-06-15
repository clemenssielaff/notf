#pragma once

#include <atomic>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/time.hpp"

NOTF_OPEN_NAMESPACE

namespace access { // forwards
template<class>
class _Application;
} // namespace access

// ================================================================================================================== //

/// The Application class.
/// After initialization, the Application singleton is available everywhere with `Application::instance()`.
/// It also manages the lifetime of the LogHandler.
class Application {

    friend class access::_Application<Window>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    template<class T>
    using Access = access::_Application<T>;

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

    /// Exception thrown when the Application could not initialize.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(initialization_error);

    /// Exception thrown when you try to access the Application instance after it was shut down.
    NOTF_EXCEPTION_TYPE(shut_down_error);

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Constructor.
    /// @param application_args         Application arguments.
    /// @throws initialization_error    When the Application intialization failed.
    explicit Application(const Args& application_args);

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(Application);

    /// Desctructor
    ~Application();

    // initialization  --------------------------------------------------------

    /// Initializes the Application through an user-defined ApplicationInfo object.
    /// @throws initialization_error    When the Application intialization failed.
    static Application& initialize(const Args& application_args) { return _instance(application_args); }

    /// Initializes the Application using only the command line arguments passed by the OS.
    /// @throws initialization_error    When the Application intialization failed.
    static Application& initialize(const int argc, char* argv[])
    {
        Args args;
        args.argc = argc;
        args.argv = argv;
        return _instance(args);
    }

    /// @{
    /// Creates a new Window instance with the given arguments.
    WindowPtr create_window();
    WindowPtr create_window(const detail::WindowArguments& args);
    /// @}

    /// Starts the application's main loop.
    /// Can only be called once.
    /// @return  The application's return value after it has finished.
    int exec();

    // global state -----------------------------------------------------------

    /// The singleton Application instance.
    /// @returns    The Application singleton.
    /// @throws initialization_error    When the Application intialization failed.
    /// @throws shut_down_error         When this method is called after the Application was shut down.
    static Application& instance()
    {
        if (NOTF_LIKELY(is_running())) {
            return _instance();
        }
        notf_throw(shut_down_error, "You may not access the Application after it was shut down");
    }

    /// Checks if the Application was once open but is now closed.
    static bool is_running() { return s_is_running.load(); }

    /// The Application's Resource Manager.
    ResourceManager& resource_manager() { return *m_resource_manager; }

    /// The Application's thread pool.
    ThreadPool& thread_pool() { return *m_thread_pool; }

    /// The RenderManager singleton.
    RenderManager& render_manager() { return *m_render_manager; }

    /// The Application's Event Manager.
    EventManager& event_manager() { return *m_event_manager; }

    /// The Application's Timer Manager.
    TimerManager& timer_manager() { return *m_timer_manager; }

    /// Timepoint when the Application was started.
    static const timepoint_t& start_time() { return s_start_time; }

    /// Age of this Application instance.
    static duration_t age()
    {
        return (s_start_time == timepoint_t{}) ? duration_t{0} : (clock_t::now() - s_start_time);
    }

private:
    /// Static (private) function holding the actual Application instance.
    static Application& _instance(const Args& application_args = s_invalid_args)
    {
        static Application instance(application_args);
        return instance;
    }

    /// Unregisters an existing Window from this Application.
    void _unregister_window(Window* window);

    /// Shuts down the application.
    /// Is called automatically, after the last Window has been closed.
    void _shutdown();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The LogHandler thread used to format and print out log messages in a thread-safe manner.
    LogHandlerPtr m_log_handler;

    /// The Application's resource manager.
    ResourceManagerPtr m_resource_manager;

    /// The global thread pool.
    ThreadPoolPtr m_thread_pool;

    /// The RenderManager singleton.
    RenderManagerPtr m_render_manager;

    /// Event Manager singleton.
    EventManagerPtr m_event_manager;

    /// Timer Manager.
    TimerManagerPtr m_timer_manager;

    /// All Windows known the the Application.
    std::vector<WindowPtr> m_windows;

    /// Timepoint when the Application was started.
    static timepoint_t s_start_time;

    /// Flag to indicate whether the Application is currently running or not.
    static std::atomic<bool> s_is_running;

    /// Invalid arguments, used when you call `get_instance` without initializing the Application first.
    static const Args s_invalid_args;
};

// ================================================================================================================== //

template<>
class access::_Application<Window> {
    friend class notf::Window;

    /// Unregisters an existing Window from this Application.
    static void unregister(Window* window) { Application::instance()._unregister_window(window); }
};

NOTF_CLOSE_NAMESPACE
