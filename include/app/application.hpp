#pragma once

#include <utility>
#include <vector>

#include "common/debug.hpp"
#include "common/error.hpp"
#include "common/keys.hpp"

struct GLFWwindow;

namespace untitled {

using std::vector;
using std::pair;

class Window;
struct WindowInfo;

///
/// \brief Singleton instance, the basis for all UI elements.
///
class Application {

    friend class Window; // may register/unregister itself

public: // enum
    ///
    /// \brief Return codes of the Application's exec() function.
    ///
    enum class RETURN_CODE {
        SUCCESS = 0,
        FAILURE = 1,
    };

public: // methods
    ///
    /// \brief Creates a new Window for this Application.
    ///
    /// \param info WindowInfo providing initialization arguments.
    ///
    /// \return The created Window.
    ///
    Window* create_window(const WindowInfo& info);

    ///
    /// \brief Overload for create_window(const WindowInfo&) using a default WindowInfo.
    ///
    /// \return The created Window.
    ///
    Window* create_window();

    ///
    /// \brief Starts the application's main loop.
    ///
    /// \return The application's return value.
    ///
    int exec();

    ///
    /// \brief Checks if any errors occurred.
    ///
    /// \return True iff one or more unhandled errors have occured.
    ///
    bool has_errors() const { return !m_errors.empty(); }

    ///
    /// \brief Returns all unhandled errors of the Application.
    ///
    /// Afterwards, calls to has_error() will respond negative because all errors returned by this function
    /// are considered "handled".
    ///
    /// \return All unhandled errors of the Application.
    ///
    vector<Error> get_errors();

public: // static methods
    ///
    /// \brief The singleton Application instance.
    ///
    /// \return The singleton Application instance.
    ///
    static Application& get_instance()
    {
        static Application instance;
        return instance;
    }

    ///
    /// \brief Called by GLFW in case of an error.
    ///
    /// \param error    Error ID.
    /// \param message  Error message;
    ///
    static void on_error(int error, const char* message);

    ///
    /// \brief Called by GLFW when a key is pressed, repeated or released.
    ///
    /// \param glfw_window  The GLFWwindow targeted by the event.
    /// \param key          Modified key.
    /// \param scancode     May hold additional information when key is set to KEY_UNKNOWN (platform dependent).
    /// \param action       The action that triggered this callback.
    /// \param mods         Additional modifier keys.
    ///
    static void on_token_key(GLFWwindow* glfw_window, int key, int scancode, int action, int mods);

private: // methods
    ///
    /// \brief Constructor.
    ///
    explicit Application();

    ///
    /// \brief Destructor.
    ///
    ~Application();

    ///
    /// \brief Returns the Window instance associated with the given GLFW window.
    ///
    /// \param glfw_window  The GLFW window to look for.
    ///
    /// \return The Window instance associated with the given GLFW window or nullptr.
    ///
    Window* get_window(const GLFWwindow* glfw_window);

    ///
    /// \brief Closes the given window and removes it from the application.
    ///
    /// \param index    Index of the window to close.
    ///
    void close_window(size_t index);

public: // deleted methods
    ///
    /// \brief Copy constructor.
    ///
    Application(const Application&) = delete;

    ///
    /// \brief Assignment operator.
    ///
    Application& operator=(Application) = delete;

private: // fields
    ///
    /// \brief All (unhandled) errors of the Application.
    ///
    vector<Error> m_errors;

    ///
    /// \brief All Windows of this application.
    ///
    vector<pair<GLFWwindow*, Window*> > m_windows;

    ///
    /// \brief The log handler thread used to format and print out log messages in a thread-safe manner.
    ///
    LogHandler m_log_handler;
};

} // namespace untitled
