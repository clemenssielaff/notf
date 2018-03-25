#pragma once

#include <condition_variable>

#include "app/forwards.hpp"
#include "common/thread.hpp"

NOTF_OPEN_NAMESPACE

class RenderThread { // TODO: move into RenderManager - this doesn't need to be its own unit

    // methods ------------------------------------------------------------
public:
    /// Constructor.
    /// @param window   The Window that is rendered into.
    RenderThread(Window& window) : m_window(window) {}

    /// Destructor.
    ~RenderThread() { stop(); }

    /// Start the RenderThread.
    void start();

    /// Requests a redraw at the next opportunity.
    /// Does not block.
    void request_redraw();

    /// Stop the RenderThread.
    /// Blocks until the worker thread joined.
    void stop();

private:
    /// Worker method.
    void _run();

    // fields -------------------------------------------------------------
private:
    /// The Window that is rendered into.
    Window& m_window;

    /// Worker thread.
    ScopedThread m_thread;

    /// Mutex guarding the RenderThread's state.
    std::mutex m_mutex;

    /// Condition variable to wait for.
    std::condition_variable m_condition;

    /// Is used in conjunction with the condition variable to notify the thread that a new frame should be drawn.
    std::atomic_flag m_is_blocked = ATOMIC_FLAG_INIT;

    /// Is true as long at the thread should continue.
    bool m_is_running = false;
};

NOTF_CLOSE_NAMESPACE
