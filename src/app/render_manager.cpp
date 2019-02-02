#include "notf/app/render_manager.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/graph/graph.hpp"

NOTF_OPEN_NAMESPACE

// render manager =================================================================================================== //

namespace detail {

RenderManager::RenderThread::RenderThread() { m_thread.run(&RenderThread::_run, *this); }

void RenderManager::RenderThread::request_redraw(std::vector<AnyNodeHandle> windows) {
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        for (auto& node : windows) { // basic round-robin scheduling
            if (!node) { continue; }
            if (auto window = handle_cast<WindowHandle>(node); window) {
                auto itr = std::find(m_dirty_windows.begin(), m_dirty_windows.end(), window);
                if (itr == m_dirty_windows.end()) { m_dirty_windows.emplace_back(std::move(window)); }
            }
        }
    }
    m_condition.notify_one();
}

void RenderManager::RenderThread::_run() {
    NOTF_LOG_TRACE("Starting render loop");

    WindowHandle window_handle;
    while (m_is_running) {
        { // wait until the next frame is ready
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_is_running) {
                m_condition.wait(lock, [&] { return !m_dirty_windows.empty() || !m_is_running; });
            }
            if (!m_is_running) { break; }
            window_handle = std::move(m_dirty_windows.front());
            m_dirty_windows.pop_front();
        }
        if (window_handle.is_expired()) { continue; }

        // use a const reference for window access, otherwise the handle will throw a ThreadError because only the ui
        // thread is allowed to access mutable methods on a handle
        const WindowHandle& window = window_handle;

        // TODO: Improve Render target ordering
        // when topological sorting, sort all free nodes into "level" buckets and sort them in a way that minimizes
        // state changes locally. The point is, the problem is NP complete or whatever, so we won't solve it or the
        // solution will take more time that in saves. Settle for something quick that improves the solution instead of
        // brute-forcing the optimal one.

        { // render the window
            GraphicsContext& context = window->get_graphics_context();
            NOTF_GUARD(context.make_current());

            context.begin_frame();
            try {
                if (SceneHandle scene = window->get_scene()) { SceneHandle::AccessFor<RenderManager>::draw(scene); }
            }
            // if an error bubbled all the way up here, something has gone horribly wrong
            catch (const notf_exception& error) {
                NOTF_LOG_ERROR("Rendering failed: \"{}\"", error.what());
            }
            context.finish_frame();
        }
    }
    NOTF_LOG_TRACE("Finished render loop");
}

void RenderManager::RenderThread::_stop() {
    m_is_running.store(false);
    m_condition.notify_one();
    m_thread.join();
}

void RenderManager::render() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    if (std::vector<AnyNodeHandle> dirty_windows = TheGraph()->synchronize(); !dirty_windows.empty()) {
        m_render_thread->request_redraw(std::move(dirty_windows));
    }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
