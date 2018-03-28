#include "app/render_manager.hpp"

#include "app/window.hpp"
#include "graphics/core/graphics_context.hpp"

//#include "common/log.hpp"
//#include "utils/reverse_iterator.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

void RenderManager::RenderThread::start()
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        if (m_is_running) {
            return;
        }
        m_is_running = true;
        m_windows.clear();
    }
    m_thread = ScopedThread(std::thread(&RenderThread::_run, this));
}

void RenderManager::RenderThread::request_redraw(Window* window)
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        for (auto& existing : m_windows) {
            if (window == existing) {
                return;
            }
        }
        m_windows.emplace_back(window);
    }
    m_condition.notify_one();
}

void RenderManager::RenderThread::stop()
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        if (!m_is_running) {
            return;
        }
        m_is_running = false;
        m_windows.clear();
    }
    m_condition.notify_one();
    m_thread = {};
}

void RenderManager::RenderThread::_run()
{
    while (1) {
        { // wait until the next frame is ready
            std::unique_lock<std::mutex> lock_guard(m_mutex);
            if (m_windows.empty()) {
                m_condition.wait(lock_guard, [&] { return !m_windows.empty() || !m_is_running; });
            }

            if (!m_is_running) {
                return;
            }
        }

        Window* window = m_windows.front();
        m_windows.pop_front();

        GraphicsContext& context = window->graphics_context();
        context.make_current();

        //    SceneManager& scene_manager = m_window.scene_manager();

        //        // ignore default state
        //        if (!scene_manager.current_state().is_valid()) {
        //            log_trace << "Cannot render a SceneManager in its invalid State";
        //            continue;
        //        }

        //        // TODO: clean all the render targets here
        //        // in order to sort them, use typeid(*ptr).hash_code()

        context.begin_frame();

        //        try {
        //            // render all Layers from back to front
        //            for (const LayerPtr& layer : reverse(scene_manager.current_state().layers)) {
        //                layer->render();
        //            }
        //        }
        //        // if an error bubbled all the way up here, something has gone horribly wrong
        //        catch (const notf_exception& error) {
        //            log_critical << "Rendering failed: \"" << error.what() << "\"";
        //        }

        context.finish_frame();
    }
}

//====================================================================================================================//

RenderManager::RenderManager() : m_render_thread(std::make_unique<RenderThread>()) { m_render_thread->start(); }

RenderManager::~RenderManager()
{
    m_render_thread.reset(); // blocks until the thread has joined
}

void RenderManager::render(Window* window) { m_render_thread->request_redraw(window); }

NOTF_CLOSE_NAMESPACE