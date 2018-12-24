#include "notf/app/graph/window.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/thread.hpp"

#include "notf/reactive/trigger.hpp"

#include "notf/graphic/glfw.hpp"

#include "notf/app/application.hpp"
#include "notf/app/graph/graph.hpp"
#include "notf/app/graph/root_node.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

/// Property policies.
using Resolution = detail::window_policy::Resolution;
using Position = detail::window_policy::Position;
using Monitor = detail::window_policy::Monitor;

GLFWmonitor* get_glfw_monitor(int index) {
    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    NOTF_ASSERT(index < count);
    return monitors[index];
}

} // namespace

// window =========================================================================================================== //

NOTF_OPEN_NAMESPACE

Window::Window(valid_ptr<AnyNode*> parent, valid_ptr<GLFWwindow*> window, Arguments settings)
    : super_t(parent), m_glfw_window(window) {
    glfwSetWindowUserPointer(m_glfw_window, this);

    // position the window
    if (settings.state == State::MINIMIZED || settings.state == State::MAXIMIZED) {
        _on_state_change(settings.state);
    } else if (settings.state == State::WINDOWED && settings.position != Position::default_value) {
        glfwSetWindowPos(m_glfw_window, settings.position.x(), settings.position.y());
    }

    // show the window after setting it up
    if (settings.state != State::FULLSCREEN && settings.is_visible) { glfwShowWindow(m_glfw_window); }

    // create the graphics context
    m_graphics_context = std::make_unique<GraphicsContext>(m_glfw_window);
    auto context_guard = m_graphics_context->make_current();

    // store settings in properties
    set<title>(std::move(settings.title));
    set<icon>(std::move(settings.icon));
    set<size>(std::move(settings.size));
    set<position>(std::move(settings.position));
    set<resolution>(std::move(settings.resolution));
    set<state>(settings.state);
    set<monitor>(settings.monitor);

    // connect property callbacks
    _set_property_callback<state>([this](State& new_state) { return _on_state_change(new_state); });
    _set_property_callback<size>([this](Size2i& new_size) { return _on_size_change(new_size); });
    _set_property_callback<resolution>([this](Size2i& new_res) { return _on_resolution_change(new_res); });
    _set_property_callback<monitor>([this](int& new_monitor) { return _on_monitor_change(new_monitor); });

    // connect slots
    m_pipe_to_close = make_pipeline(_get_slot<to_close>() | Trigger([&]() {
                                        NOTF_LOG_TRACE("Closing Window \"{}\"", get<title>());
                                        remove();
                                    }));
    NOTF_LOG_INFO("Created Window \"{}\" using OpenGl version: {}", get<title>(), glGetString(GL_VERSION));

    // finally, register with the application
    TheApplication::AccessFor<Window>::register_window(m_glfw_window);
}

WindowHandle Window::create(Arguments settings) {
    if (NOTF_UNLIKELY(!this_thread::is_the_ui_thread())) {
        NOTF_THROW(ThreadError, "Window::create must only be called from the UI thread");
    }

    _validate_settings(settings);
    GLFWwindow* glfw_window = nullptr;
    {
        if (this_thread::is_the_main_thread()) {
            glfw_window = _create_glfw_window(settings);
        } else {
            fibers::promise<GLFWwindow*> promise;
            auto future = promise.get_future();
            TheApplication()->schedule([&settings, promise = std::move(promise)]() mutable {
                promise.set_value(Window::_create_glfw_window(settings));
            });
            glfw_window = future.get();
        }
    }
    if (!glfw_window) {
        NOTF_THROW(InitializationError, "Window or OpenGL context creation failed for Window \"{}\"", settings.title);
    }

    RootNodePtr root_node = TheGraph::AccessFor<Window>::get_root_node_ptr();
    WindowPtr window = _create_shared(root_node.get(), glfw_window, std::move(settings));
    AnyNode::AccessFor<Window>::finalize(*window);

    RootNode::AccessFor<Window>::add_window(*root_node, window);
    TheGraph::AccessFor<Window>::register_node(std::static_pointer_cast<AnyNode>(window));

    // TODO: why doesn't the RootNode create Windows?

    return window;
}

Window::~Window() {
    // unregister from the application and event handling
    TheApplication::AccessFor<Window>::unregister_window(m_glfw_window);
}

void Window::_validate_settings(Arguments& settings) {
    // warn about ignored settings
    if (settings.state == State::FULLSCREEN) {
        if (!settings.is_visible) {
            NOTF_LOG_WARN("Ignoring setting \"is_visible = false\" for Window \"{}\", "
                          "because the Window was requested to start in full screen",
                          settings.title);
            settings.is_visible = true;
        }
        if (settings.is_decorated) {
            NOTF_LOG_WARN("Ignoring setting \"is_decorated = true\" for Window \"{}\", "
                          "because the Window was requested to start in full screen",
                          settings.title);
            settings.is_decorated = false;
        }
        if (!settings.is_focused) {
            NOTF_LOG_WARN("Ignoring setting \"is_focused = false\" for Window \"{}\", "
                          "because the Window was requested to start in full screen",
                          settings.title);
            settings.is_focused = true;
        }
    }
    if (!settings.is_visible) {
        if (settings.is_focused) {
            NOTF_LOG_WARN("Ignoring setting \"is_focused = true\" for Window \"{}\", "
                          "because the Window was requested to start invisible",
                          settings.title);
            settings.is_focused = false;
        }
    }

    if (settings.monitor != -1) { // make sure the monitor is valid
        int count = 0;
        glfwGetMonitors(&count);
        if (count == 0) { NOTF_THROW(Window::InitializationError, "Cannot find a monitor to place the Window on to"); }
        if (settings.monitor < count) {
            NOTF_LOG_WARN("Cannot place Window \"{}\" on monitor {} because the system only provides {}. "
                          "Using primary monitor instead.",
                          settings.title, settings.monitor, count);
            settings.monitor = 0;
        }
    }

    // correct nonsensical values
    settings.samples = max(0, settings.samples);
}

GLFWwindow* Window::_create_glfw_window(const Arguments& settings) {

    // Window specific GLFW hints (see Application constructor for application-wide hints)
    glfwWindowHint(GLFW_VISIBLE, settings.is_visible);
    glfwWindowHint(GLFW_FOCUSED, settings.is_focused);
    glfwWindowHint(GLFW_DECORATED, settings.is_decorated);
    glfwWindowHint(GLFW_RESIZABLE, settings.is_resizeable);
    glfwWindowHint(GLFW_SAMPLES, settings.samples);
    glfwWindowHint(GLFW_MAXIMIZED, settings.state == State::MAXIMIZED);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // always start out invisible

    GLFWmonitor* window_monitor = nullptr;
    Size2i window_size = settings.size;
    if (settings.state == State::FULLSCREEN) {

        // fullscreen windows adopt the system resolution by default, but can use a manually specified resolution
        if (settings.resolution != Resolution::default_value) {
            window_size = settings.resolution;
        } else {
            const GLFWvidmode* mode = glfwGetVideoMode(window_monitor);
            window_size = {mode->width, mode->height};
        }

        // find the monitor to display the Window
        if (settings.monitor != Monitor::default_value) {
            window_monitor = get_glfw_monitor(settings.monitor);
            if (!window_monitor) {
                window_monitor = get_glfw_monitor(0);
                if (window_monitor) {
                    NOTF_LOG_WARN("Cannot place Window \"{}\" on unknown monitor {}, using default monitor instead",
                                  settings.title, settings.monitor);
                }
            }
        } else {
            window_monitor = get_glfw_monitor(0);
        }
        if (!window_monitor) {
            NOTF_THROW(InitializationError, "Failed to find a monitor to display Window \"{}\"", settings.title);
        }
    }

    return glfwCreateWindow(window_size.width(), window_size.height(), settings.title, window_monitor,
                            TheApplication::AccessFor<Window>::get_shared_context());
}

void Window::_move_to_monitor(GLFWmonitor* window_monitor) {
    const GLFWvidmode* video_mode = glfwGetVideoMode(window_monitor);
    Size2i buffer_size;
    if (get<resolution>() != Resolution::default_value) {
        buffer_size = get<resolution>();
    } else {
        buffer_size = {video_mode->width, video_mode->height};
    }
    glfwSetWindowMonitor(m_glfw_window, window_monitor, /*xpos*/ GLFW_DONT_CARE, /*ypos*/ GLFW_DONT_CARE,
                         buffer_size.width(), buffer_size.height(), video_mode->refreshRate);
}

bool Window::_on_state_change(Arguments::State& new_state) {
    { // windowify the window first, if it is currently in full screen
        GLFWmonitor* glfw_monitor = glfwGetWindowMonitor(m_glfw_window);
        if (glfw_monitor && (new_state != State::FULLSCREEN)) {
            const V2i& window_pos = get<position>();
            const Size2i& window_size = get<size>();
            glfwSetWindowMonitor(m_glfw_window, /*monitor*/ nullptr, window_pos.x(), window_pos.y(), //
                                 window_size.width(), window_size.height(), /*refresh rate*/ GLFW_DONT_CARE);
        }
    }

    switch (new_state) {
    case State::MINIMIZED: glfwIconifyWindow(m_glfw_window); break;
    case State::WINDOWED: glfwRestoreWindow(m_glfw_window); break;
    case State::MAXIMIZED: glfwMaximizeWindow(m_glfw_window); break;
    case State::FULLSCREEN: _move_to_monitor(get_glfw_monitor(get<monitor>())); break;
    }

    return true; // always succeed
}

bool Window::_on_size_change(Size2i& new_size) {
    new_size.max(Size2i::zero());

    if (get<state>() == State::WINDOWED) { glfwSetWindowSize(m_glfw_window, new_size.width(), new_size.height()); }

    return true; // always succeed
}

bool Window::_on_resolution_change(Size2i& new_resolution) {
    new_resolution.max(Size2i::zero());

    if (get<state>() == State::FULLSCREEN) {
        glfwSetWindowSize(m_glfw_window, new_resolution.width(), new_resolution.height());
    }

    return true; // always succeed
}

bool Window::_on_monitor_change(int& new_monitor) {
    int monitor_count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
    if (new_monitor < 0 || new_monitor >= monitor_count) {
        NOTF_LOG_WARN("Cannot place Window \"{}\" on unknown monitor {}, using default monitor instead", get<title>(),
                      new_monitor);
        new_monitor = 0;
    }

    if (get<state>() == State::FULLSCREEN) { _move_to_monitor(monitors[new_monitor]); }

    return true; // always succeed
}

NOTF_CLOSE_NAMESPACE
