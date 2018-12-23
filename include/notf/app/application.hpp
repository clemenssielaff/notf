#pragma once

#include "notf/meta/singleton.hpp"

#include "notf/common/delegate.hpp"
#include "notf/common/fibers.hpp"
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

        // flags --------------------------------------------------------------

        /// If true, `Application::exec` will always start its main loop on , even if no Windows have been created yet.
        /// In that case, you'll have to manually call shutdown from another thread in order to close the Application in
        /// an orderly fashion. By default, this flag is set to false, meaning that `Application::exec` with no Windows
        /// immediately returns without blocking.
        bool start_without_windows = false;

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

        /// Number of unhandled Events before the Application blocks enqueuing new ones (must be a power of two).
        size_t app_buffer_size = 16;
    };

private:
    /// State of the Application: UNSTARTED -> RUNNING -> CLOSED
    enum class State {
        UNSTARTED,
        RUNNING,
        CLOSED,
    };

    /// Object containing functions passed to execute on the main thread.
    struct AnyAppEvent {
        virtual ~AnyAppEvent() = default;
        virtual void run() = 0;
    };
    template<class Func>
    struct AppEvent : public AnyAppEvent {
        AppEvent(Func&& function) : m_function(std::forward<Func>(function)) {}
        void run() final {
            if (m_function) { std::invoke(m_function); }
        }
        Delegate<void()> m_function;
    };
    using AnyAppEventPtr = std::unique_ptr<AnyAppEvent>;

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

    /// @{
    /// Schedules a new event to be handled on the main thread.
    /// You can schedule events prior to calling `exec` and they will be executed in the first run of the main loop.
    /// Anything scheduled after shutdown is ignored and will never be executed.
    void schedule(AnyAppEventPtr&& event);
    template<class Func>
    std::enable_if_t<std::is_invocable_v<Func>> schedule(Func&& function) {
        schedule(std::make_unique<AppEvent<Func>>(std::forward<Func>(function)));
    }
    /// @}

    /// Forces a shutdown of a running Application.
    /// Does nothing if the Application is already shut down.
    void shutdown();

private:
    /// Registers a new Window in the Application.
    void _register_window(GLFWwindow* window);

    /// Unregisters an existing Window from this Application.
    void _unregister_window(GLFWwindow* window);

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

    /// MPMC queue buffering Events for the main thread.
    fibers::buffered_channel<AnyAppEventPtr> m_event_queue;

    /// All Windows known the the Application.
    std::unique_ptr<std::vector<detail::GlfwWindowPtr>> m_windows;
    Mutex m_windows_mutex;

    /// @{
    /// ScopedSingleton holders.
    /// The following objects only initialize and control the lifetime of ScopedSingletons that are available from
    /// anywhere in the code, as long as the Application instance lives.
    /// There is no need to get them out of the Application, just call `TheEventHandler()->...` or whatever to use them.
    std::unique_ptr<TheEventHandler> m_event_handler;
    std::unique_ptr<TheTimerPool> m_timer_pool;
    std::unique_ptr<TheGraph> m_graph;
    std::unique_ptr<TheGraphicsSystem> m_graphics_system;
    std::unique_ptr<TheRenderManager> m_render_manager;
    /// @}

    /// State of the Application: UNSTARTED -> RUNNING -> CLOSED
    std::atomic<State> m_state = State::UNSTARTED;
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
    void _register_window(GLFWwindow* window) {
        NOTF_ASSERT(_is_this_the_ui_thread());
        _get()._register_window(window);
    }

    /// Unregisters an existing Window from this Application.
    void _unregister_window(GLFWwindow* window) {
        NOTF_ASSERT(_is_this_the_ui_thread());
        _get()._unregister_window(window);
    }

    /// Tests if the calling thread is the notf "UI-thread".
    bool _is_this_the_ui_thread() {
        RecursiveMutex& ui_mutex = _get().m_ui_mutex;
        if (ui_mutex.try_lock()) {
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
    static void register_window(GLFWwindow* window) { TheApplication()._register_window(window); }

    /// Unregisters an existing Window from this Application.
    static void unregister_window(GLFWwindow* window) { TheApplication()._unregister_window(window); }

    /// The internal GLFW window managed by the Application holding the shared context.
    static GLFWwindow* get_shared_context() { return TheApplication()._get_shared_context(); }
};

// this_thread (injection) ========================================================================================== //

namespace this_thread {

/// Tests if the calling thread is the notf "UI-thread".
bool is_the_ui_thread();

} // namespace this_thread

NOTF_CLOSE_NAMESPACE
