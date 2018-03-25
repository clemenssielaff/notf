#include "app/render_thread.hpp"

#include "app/scene_manager.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "utils/reverse_iterator.hpp"

NOTF_OPEN_NAMESPACE

void RenderThread::start()
{
    GraphicsContext& graphics_context = m_window.graphics_context();
    graphics_context.release_current();
    {
        std::unique_lock<std::mutex> lock_guard(m_mutex);
        if (m_is_running) {
            return;
        }
        m_is_running = true;
    }
    m_is_blocked.test_and_set(std::memory_order_release);
    m_thread = ScopedThread(std::thread(&RenderThread::_run, this));
}

void RenderThread::request_redraw()
{
    m_is_blocked.clear(std::memory_order_release);
    m_condition.notify_one();
}

void RenderThread::stop()
{
    {
        std::unique_lock<std::mutex> lock_guard(m_mutex);
        if (!m_is_running) {
            return;
        }
        m_is_running = false;
        m_is_blocked.clear(std::memory_order_release);
    }
    m_condition.notify_one();
    m_thread = {};
}

void RenderThread::_run()
{
    GraphicsContext& graphics_context = m_window.graphics_context();
    graphics_context.make_current();

    SceneManager& scene_manager = m_window.scene_manager();

    while (1) {
        { // wait until the next frame is ready
            std::unique_lock<std::mutex> lock_guard(m_mutex);
            if (m_is_blocked.test_and_set(std::memory_order_acquire)) {
                m_condition.wait(lock_guard, [&] { return !m_is_blocked.test_and_set(std::memory_order_acquire); });
            }

            if (!m_is_running) {
                return;
            }
        }

        // ignore default state
        if (!scene_manager.current_state().is_valid()) {
            log_trace << "Cannot render a SceneManager in its invalid State";
            continue;
        }

        // TODO: clean all the render targets here
        // in order to sort them, use typeid(*ptr).hash_code()

        graphics_context.begin_frame();

        try {
            // render all Layers from back to front
            for (const LayerPtr& layer : reverse(scene_manager.current_state().layers)) {
                //                layer->render();
            }
        }
        // if an error bubbled all the way up here, something has gone horribly wrong
        catch (const notf_exception& error) {
            log_critical << "Rendering failed: \"" << error.what() << "\"";
        }

        graphics_context.finish_frame();
    }
}

NOTF_CLOSE_NAMESPACE
