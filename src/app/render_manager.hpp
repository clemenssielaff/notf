#pragma once

#include <condition_variable>
#include <deque>

#include "app/forwards.hpp"
#include "common/mutex.hpp"
#include "common/thread.hpp"

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

/// The single RenderManager of an Application.
class RenderManager {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// Internal worker thread waiting for Windows to render.
    class RenderThread {

        // methods ------------------------------------------------------------
    public:
        /// Destructor.
        ~RenderThread() { stop(); }

        /// Start the RenderThread.
        void start();

        /// Requests a redraw of the given Window at the next opportunity.
        /// @param window   Window to redraw.
        void request_redraw(Window* window);

        /// Stop the RenderThread.
        /// Blocks until the worker thread joined.
        void stop();

    private:
        /// Worker thread method.
        void _run();

        // fields -------------------------------------------------------------
    private:
        /// Worker thread.
        ScopedThread m_thread;

        /// Windows to render.
        std::deque<Window*> m_windows;

        /// Mutex guarding the RenderThread's state.
        Mutex m_mutex;

        /// Condition variable to wait for.
        ConditionVariable m_condition;

        /// Is true as long at the thread is alive.
        bool m_is_running = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(RenderManager);

    /// Default constructor.
    RenderManager();

    /// Destructor.
    ~RenderManager();

    /// Renders the given Window at the next opportunity.
    /// @param window   Window to render.
    void render(Window* window);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Internal worker thread waiting for Windows to render.
    std::unique_ptr<RenderThread> m_render_thread;
};

NOTF_CLOSE_NAMESPACE
