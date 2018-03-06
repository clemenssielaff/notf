#include "app/scene/layer_manager.hpp"

#include <sstream>

#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/renderer/graphics_producer.hpp"
#include "app/renderer/render_target.hpp"
#include "app/scene/layer.hpp"
#include "app/scene/scene.hpp"
#include "common/hash.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/text/font_manager.hpp"
#include "utils/make_smart_enabler.hpp"
#include "utils/reverse_iterator.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

void RenderDag::add(const GraphicsProducerId producer, const RenderTargetId target)
{
    m_new_hash = hash(m_new_hash, producer, target);
    m_dependencies.emplace_back(std::make_pair(producer, target));
}

// const std::vector<RenderTargetId>& RenderTargetDependencies::sort()
//{
//    // early out if nothing has changed.
////    if (m_new_hash == m_last_hash) {
////        return m_dag.last_result();
////    }
////    m_dag.reinit()

////        // TODO: continue here by sorting stuff

////        return m_sorted_targets;
//}

void RenderDag::reset() { m_new_hash = 0; }

} // namespace detail

//====================================================================================================================//

void LayerManager::RenderThread::start()
{
    {
        std::unique_lock<std::mutex> lock_guard(m_mutex);
        if (m_is_running) {
            return;
        }
        m_is_running = true;
    }
    m_is_blocked.test_and_set(std::memory_order_release);
    m_thread = ScopedThread(std::thread(&LayerManager::RenderThread::_run, this));
}

void LayerManager::RenderThread::request_redraw()
{
    m_is_blocked.clear(std::memory_order_release);
    m_condition.notify_one();
}

void LayerManager::RenderThread::stop()
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

void LayerManager::RenderThread::_run()
{
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
        if (m_manager.m_state == &s_default_state) {
            log_trace << "Cannot render a LayerManager in its default State";
            continue;
        }

        // TODO: clean all the render targets here
        // in order to sort them, use typeid(*ptr).hash_code()

        m_manager.m_graphics_context->begin_frame();

        try {
            // render all Layers from back to front
            for (const LayerPtr& layer : reverse(m_manager.m_state->layers)) {
                layer->render();
            }
        }
        // if an error bubbled all the way up here, something has gone horribly wrong
        catch (const notf_exception& error) {
            log_critical << "Rendering failed: \"" << error.what() << "\"";
        }

        m_manager.m_graphics_context->finish_frame();
    }
}

//====================================================================================================================//

const LayerManager::State LayerManager::s_default_state = {};

LayerManager::LayerManager(GLFWwindow* window)
    : m_render_thread(*this)
    , m_graphics_context(std::make_unique<GraphicsContext>(window)) // TODO: move GraphicsContext and FontManager to
                                                                    // Window
    , m_font_manager(FontManager::create(*m_graphics_context))
    , m_dependencies()
    , m_graphics_producer()
    , m_render_targets()
    , m_state(&s_default_state)
{
    m_render_thread.start();
}

LayerManagerPtr LayerManager::create(GLFWwindow* window)
{
#ifdef NOTF_DEBUG
    return LayerManagerPtr(new LayerManager(window));
#else
    return std::make_unique<make_shared_enabler<LayerManager>>(window);
#endif
}

LayerManager::~LayerManager() = default;

LayerManager::StateId LayerManager::add_state(State&& state)
{
    StateId new_id = _next_id();
    m_states.emplace(std::make_pair(new_id, std::move(state)));
    return new_id;
}

const LayerManager::State& LayerManager::state(const StateId id) const
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "LayerManager has no State with the ID \"" << id << "\"");
    }
    return it->second;
}

void LayerManager::enter_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "LayerManager has no State with the ID \"" << id << "\"");
    }
    m_state = &it->second;
}

void LayerManager::remove_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "LayerManager has no State with the ID \"" << id << "\"");
    }
    if (m_state == &it->second) {
        log_warning << "Removing current LayerManager state \"" << it->first
                    << "\" - falling back to the default state";
        m_state = &s_default_state;
    }
    m_states.erase(it);
}

void LayerManager::propagate(MouseEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void LayerManager::propagate(KeyEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void LayerManager::propagate(CharEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void LayerManager::resize(Size2i size)
{
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->resize(size);
    }
}

void LayerManager::_register_new(GraphicsProducerPtr graphics_producer)
{
    if (m_graphics_producer.count(graphics_producer->id())) {
        notf_throw_format(runtime_error, "Failed to register GraphicsProducer with duplicate ID: \""
                                             << graphics_producer->id() << "\"");
    }
    m_graphics_producer.emplace(std::make_pair(graphics_producer->id(), graphics_producer));
}

void LayerManager::_register_new(RenderTargetPtr render_target)
{
    if (m_render_targets.count(render_target->id())) {
        notf_throw_format(runtime_error,
                          "Failed to register RenderTarget with duplicate ID: \"" << render_target->id() << "\"");
    }
    m_render_targets.emplace(std::make_pair(render_target->id(), render_target));
}

LayerManager::StateId LayerManager::_next_id()
{
    static LayerManager::StateId::underlying_t next = 1;
    return LayerManager::StateId(next++);
}

NOTF_CLOSE_NAMESPACE