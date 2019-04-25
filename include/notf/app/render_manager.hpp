#pragma once

#include <deque>

#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/graph/window.hpp" // TODO: I don't like that render_manager.hpp includes window.hpp

NOTF_OPEN_NAMESPACE

// render manager =================================================================================================== //

namespace detail {

/// The single RenderManager of an Application.
class RenderManager {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// Internal worker thread, running in parallel to the ui thread.
    class RenderThread {

        // methods ------------------------------------------------------------
    public:
        /// Default constructor.
        RenderThread();

        /// Destructor.
        ~RenderThread() { _stop(); }

        /// Requests a redraw of the given Window at the next opportunity.
        /// @param windows  Window to redraw.
        void request_redraw(std::vector<AnyNodeHandle> windows);

    private:
        /// Worker thread method.
        void _run();

        /// Stop the RenderThread.
        /// Blocks until the worker thread joined.
        void _stop();

        // fields -------------------------------------------------------------
    private:
        /// Worker thread.
        Thread m_thread;

        /// Mutex guarding the RenderThread's state.
        std::mutex m_mutex;

        /// Condition variable to wait for.
        std::condition_variable m_condition;

        /// Windows that need to be re-rendered.
        std::deque<WindowHandle> m_dirty_windows;

        /// Is true as long at the thread is alive.
        std::atomic_bool m_is_running = true;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(RenderManager);

    /// Default constructor.
    RenderManager() : m_render_thread(std::make_unique<RenderThread>()) {}

    /// Renders the given Window at the next opportunity.
    void render();

    // fields  ------------------------------------------------------------------------------------------------------ //
private:
    /// Internal worker thread waiting for Windows to render.
    std::unique_ptr<RenderThread> m_render_thread;
};

} // namespace detail

// the render mananger ============================================================================================== //

class TheRenderManager : public ScopedSingleton<detail::RenderManager> {

    friend Accessor<TheRenderManager, detail::Application>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheRenderManager);

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(TheRenderManager);

public:
    using ScopedSingleton<detail::RenderManager>::ScopedSingleton;
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheRenderManager, detail::Application> {
    friend detail::Application;

    /// Creates the ScopedSingleton holder instance of TheRenderManager.
    template<class... Args>
    static auto create(Args... args) {
        return TheRenderManager::_create_unique(TheRenderManager::Holder{}, std::forward<Args>(args)...);
    }
};

NOTF_CLOSE_NAMESPACE
