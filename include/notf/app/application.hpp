#pragma once

#include "notf/meta/singleton.hpp"

#include "notf/common/mutex.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// application ====================================================================================================== //

namespace detail {

class Application {

    friend TheApplication;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Exception thrown when the Application could not initialize.
    /// The error string will contain more detailed information about the error.
    NOTF_EXCEPTION_TYPE(StartupError);

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
public:
    NOTF_NO_COPY_OR_ASSIGN(Application);

    /// Constructor.
    /// @param args             Application arguments.
    Application(Arguments args);

    /// Destructor.
    ~Application();

    /// Application arguments as passed to the constructor.
    Arguments& get_arguments() { return m_arguments; }

    /// Application-wide Scheduler.
    Scheduler& get_scheduler() { return *m_scheduler; }

    /// Starts the application's main loop.
    /// @return  The application's return value after it has finished.
    /// @throws StartupError    When the Application is already running.
    /// @throws ShutdownError   When this method is called after the Application was shut down.
    int exec();

    /// Forces a shutdown of a running Application.
    /// Does nothing if the Application is already shut down.
    void shutdown();

private:
    /// Registers a new Window in the Application.
    void _register_window(WindowPtr window);

    /// Unregisters an existing Window from this Application.
    void _unregister_window(WindowPtr window);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Application arguments as passed to the constructor.
    Arguments m_arguments;

    /// The internal GLFW window managed by the Application
    /// Does not actually open a Window, only provides the shared OpenGL context.
    detail::GlfwWindowPtr m_shared_context{nullptr, detail::window_deleter};

    /// Mutex protecting the list of registered Windows.
    Mutex m_window_mutex;

    /// All Windows known the the Application.
    std::unique_ptr<std::vector<WindowPtr>> m_windows;

    /// Application-wide event scheduler.
    SchedulerPtr m_scheduler;

    std::atomic_flag m_is_running = ATOMIC_FLAG_INIT;
    std::atomic_flag m_should_continue = ATOMIC_FLAG_INIT;

    /// Is true if any of the Application's Windows have been closed and need to be destroyed on the main thread.
    std::atomic_bool m_has_windows_to_close = false;
};

}; // namespace detail

// the application ================================================================================================== //

class TheApplication : public ScopedSingleton<detail::Application> {

    friend Accessor<TheApplication, Window>;

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base type.
    using super_t = ScopedSingleton<detail::Application>;

public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheApplication);

    /// Exception thrown when the Application could not initialize.
    /// The error string will contain more detailed information about the error.
    using StartupError = detail::Application::StartupError;

    /// Application arguments.
    using Arguments = detail::Application::Arguments;

    // methods --------------------------------------------------------------------------------- //
public:
    using super_t::super_t; // use baseclass' constructors

private:
    /// The internal GLFW window managed by the Application holding the shared context.
    GLFWwindow* _get_shared_context() { return _get().m_shared_context.get(); }

    /// Registers a new Window in the Application.
    void _register_window(WindowPtr window) { _get()._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    void _unregister_window(WindowPtr window) { _get()._unregister_window(std::move(window)); }
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheApplication, Window> {
    friend Window;

    /// Registers a new Window in the Application.
    static void register_window(WindowPtr window) { TheApplication()._register_window(std::move(window)); }

    /// Unregisters an existing Window from this Application.
    static void unregister_window(WindowPtr window) { TheApplication()._unregister_window(std::move(window)); }

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* get_shared_context() { return TheApplication()._get_shared_context(); }
};

NOTF_CLOSE_NAMESPACE
