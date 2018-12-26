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
            if (auto window = node->to_handle<Window>(); window) {
                auto itr = std::find(m_dirty_windows.begin(), m_dirty_windows.end(), window);
                if (itr == m_dirty_windows.end()) { m_dirty_windows.emplace_back(std::move(window)); }
            }
        }
    }
    m_condition.notify_one();
}

void RenderManager::RenderThread::_run() {
    WindowHandle window;
    while (m_is_running) {
        { // wait until the next frame is ready
            std::unique_lock<Mutex> lock(m_mutex);
            if (m_is_running) {
                m_condition.wait(lock, [&] { return !m_dirty_windows.empty() || !m_is_running; });
            }
            if (!m_is_running) { return; }
            window = std::move(m_dirty_windows.front());
            m_dirty_windows.pop_front();
        }
        { // render the window
            GraphicsContext& context = window->get_graphics_context();
            const auto context_guard = context.make_current();

            context.begin_frame();
            try {
                window->get_scene()->draw();
            }
            // if an error bubbled all the way up here, something has gone horribly wrong
            catch (const notf_exception& error) {
                NOTF_LOG_ERROR("Rendering failed: \"{}\"", error.what());
            }
            context.finish_frame();
        }
    }
}

void RenderManager::RenderThread::_stop() {
    m_is_running.store(false);
    m_condition.notify_one();
    m_thread.join();
}

void RenderManager::render() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    std::vector<AnyNodeHandle> dirty_windows = TheGraph()->synchronize();
    if (!dirty_windows.empty()) { m_render_thread->request_redraw(std::move(dirty_windows)); }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
