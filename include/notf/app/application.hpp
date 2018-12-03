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

        // buffer sizes -------------------------------------------------------

        /// Number of unhandled Events before the EventHandler blocks enqueuing new ones (must be a power of two).
        size_t event_buffer_size = 128;

        /// Number of unscheduled Timers before the TimerPool blocks enqueuing new ones (must be a power of two).
        size_t timer_buffer_size = 32;
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

    /// Mutex determining which thread is the UI-thread.
    RecursiveMutex m_ui_mutex;

    /// Lock held by the UI-thread.
    std::unique_lock<RecursiveMutex> m_ui_lock;

    /// All Windows known the the Application.
    std::unique_ptr<std::vector<WindowPtr>> m_windows;
    Mutex m_windows_mutex;

    /// @{
    /// ScopedSingleton holders.
    /// The following objects only initialize and control the lifetime of ScopedSingletons that are available from
    /// anywhere in the code, as long as the Application instance lives.
    /// There is no need to get them out of the Application, just call `TheEventHandler()->...` or whatever to use them.
    std::unique_ptr<TheEventHandler> m_event_handler;
    std::unique_ptr<TheTimerPool> m_timer_pool;
    std::unique_ptr<TheGraph> m_graph;
    /// @}

    /// Set once at the beginning of `exec` to make sure the main loop is only started once.
    std::atomic_flag m_is_running = ATOMIC_FLAG_INIT;

    /// Is set to true at the beginning of the main loop and cleared by the user when it is time to stop.
    std::atomic_flag m_should_continue;

    /// Is false if any one of the Application's Windows has been closed and needs to be destroyed on the main thread.
    std::atomic_flag m_are_windows_intact = ATOMIC_FLAG_INIT;
};

}; // namespace detail

// the application ================================================================================================== //

class TheApplication : public ScopedSingleton<detail::Application> {

    friend Accessor<TheApplication, Window>;
    friend bool this_thread::is_the_ui_thread();

    // types ----------------------------------------------------------------------------------- //
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
    using ScopedSingleton<detail::Application>::ScopedSingleton;

    /// Allow the user to construct the holder-singleton instance.
    template<class... Args>
    TheApplication(Args&&... args) : ScopedSingleton<detail::Application>(Holder{}, std::forward<Args>(args)...) {}

private:
    /// The internal GLFW window managed by the Application holding the shared context.
    GLFWwindow* _get_shared_context() { return _get().m_shared_context.get(); }

    /// Registers a new Window in the Application.
    void _register_window(WindowPtr window) {
        NOTF_ASSERT(_is_this_the_ui_thread());
        _get()._register_window(std::move(window));
    }

    /// Unregisters an existing Window from this Application.
    void _unregister_window(WindowPtr window) {
        NOTF_ASSERT(_is_this_the_ui_thread());
        _get()._unregister_window(std::move(window));
    }

    /// Tests if the calling thread is the notf "UI-thread".
    bool _is_this_the_ui_thread() { //return std::unique_lock(_get().m_ui_mutex).try_lock(); }
        RecursiveMutex& ui_mutex = _get().m_ui_mutex;
        if(ui_mutex.try_lock()){
            ui_mutex.unlock();
            return true;
        } else {
            return false;
        }
    }
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

// this_thread (injection) ========================================================================================== //

namespace this_thread {

/// Tests if the calling thread is the notf "UI-thread".
bool is_the_ui_thread();

} // namespace this_thread

NOTF_CLOSE_NAMESPACE
