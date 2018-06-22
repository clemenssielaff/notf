#include "app/render_manager.hpp"

#include "app/layer.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "utils/reverse_iterator.hpp"

NOTF_OPEN_NAMESPACE

void RenderManager::RenderThread::start()
{
    {
        NOTF_MUTEX_GUARD(m_mutex);
        if (m_is_running) {
            return;
        }
        m_is_running = true;
        m_windows.clear();
    }
    m_thread = ScopedThread(std::thread(&RenderThread::_run, this));
}

void RenderManager::RenderThread::request_redraw(valid_ptr<WindowPtr> window)
{
    {
        NOTF_MUTEX_GUARD(m_mutex);
        for (auto& existing : m_windows) {
            if (window == existing) {
                return;
            }
        }
        m_windows.emplace_back(std::move(window));
    }
    m_condition.notify_one();
}

void RenderManager::RenderThread::stop()
{
    {
        NOTF_MUTEX_GUARD(m_mutex);
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
    //    size_t frame_counter = 0;

    while (true) {

        WindowPtr window;
        { // wait until the next frame is ready
            std::unique_lock<Mutex> lock(m_mutex);
            if (m_windows.empty() && m_is_running) {
                m_condition.wait(lock, [&] { return !m_windows.empty() || !m_is_running; });
            }

            if (!m_is_running) {
                return;
            }

            window = std::move(m_windows.front());
            m_windows.pop_front();
        }
        NOTF_ASSERT(window);
        if (window->is_closed()) {
            continue;
        }

        SceneGraphPtr& scene_graph = window->get_scene_graph();
        GraphicsContext& context = window->get_graphics_context();

        { // render the frame
            SceneGraph::FreezeGuard freeze_guard = SceneGraph::Access<RenderManager>::freeze(*scene_graph);
            GraphicsContext::CurrentGuard context_guard = context.make_current();

            // log_trace << "Rendering frame: " << frame_counter++;

            //        // TODO: clean all the render targets here
            //        // in order to sort them, use typeid(*ptr).hash_code()

            context.begin_frame();

            try {
                // render all Layers from back to front
                for (const auto& layer : reverse(scene_graph->current_state()->layers())) { //
                    layer->render();
                }
            }
            // if an error bubbled all the way up here, something has gone horribly wrong
            catch (const notf_exception& error) {
                log_critical << "Rendering failed: \"" << error.what() << "\"";
            }

            context.finish_frame();
        }
    }
}

// ================================================================================================================== //

RenderManager::RenderManager() : m_render_thread(std::make_unique<RenderThread>()) { m_render_thread->start(); }

RenderManager::~RenderManager()
{
    m_render_thread.reset(); // blocks until the thread has joined
}

void RenderManager::render(valid_ptr<WindowPtr> window) { m_render_thread->request_redraw(std::move(window)); }

NOTF_CLOSE_NAMESPACE
