#include "graphics/engine/render_manager.hpp"

#include <sstream>

#include "common/hash.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/engine/graphics_producer.hpp"
#include "graphics/engine/layer.hpp"
#include "graphics/engine/render_target.hpp"
#include "graphics/text/font_manager.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

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

const RenderManager::State RenderManager::s_default_state = {};

RenderManager::RenderManager(GLFWwindow* window)
    : m_graphics_context(std::make_unique<GraphicsContext>(window))
    , m_font_manager(FontManager::create(*m_graphics_context))
    , m_dependencies()
    , m_graphics_producer()
    , m_render_targets()
    , m_state(&s_default_state)
{}

RenderManagerPtr RenderManager::create(GLFWwindow* window)
{
#ifdef NOTF_DEBUG
    return RenderManagerPtr(new RenderManager(window));
#else
    return std::make_unique<make_shared_enabler<RenderManager>>(window);
#endif
}

RenderManager::~RenderManager() = default;

RenderManager::StateId RenderManager::add_state(State&& state)
{
    StateId new_id = _next_id();
    m_states.emplace(std::make_pair(new_id, std::move(state)));
    return new_id;
}

const RenderManager::State& RenderManager::state(const StateId id) const
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "RenderManager has no State with the ID \"" << id << "\"");
    }
    return it->second;
}

void RenderManager::enter_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "RenderManager has no State with the ID \"" << id << "\"");
    }
    m_state = &it->second;
}

void RenderManager::remove_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "RenderManager has no State with the ID \"" << id << "\"");
    }
    if (m_state == &it->second) {
        log_warning << "Removing current RenderManager state \"" << it->first
                    << "\" - falling back to the default state";
        m_state = &s_default_state;
    }
    m_states.erase(it);
}

void RenderManager::render()
{
    if (m_state == &s_default_state) {
        log_trace << "Ignoring RenderManager::render with the default State";
        return;
    }

    // TODO: clean all the render targets here

    m_graphics_context->begin_frame();

    // render all Layers
    for (const LayerPtr& layer : m_state->layers) {
        layer->render();
    }

    m_graphics_context->finish_frame();
}

void RenderManager::register_new(GraphicsProducerPtr graphics_producer)
{
    if (m_graphics_producer.count(graphics_producer->id())) {
        notf_throw_format(runtime_error, "Failed to register GraphicsProducer with duplicate ID: \""
                                             << graphics_producer->id() << "\"");
    }
    m_graphics_producer.emplace(std::make_pair(graphics_producer->id(), graphics_producer));
}

void RenderManager::register_new(RenderTargetPtr render_target)
{
    if (m_render_targets.count(render_target->id())) {
        notf_throw_format(runtime_error,
                          "Failed to register RenderTarget with duplicate ID: \"" << render_target->id() << "\"");
    }
    m_render_targets.emplace(std::make_pair(render_target->id(), render_target));
}

RenderManager::StateId RenderManager::_next_id()
{
    static RenderManager::StateId::underlying_t next = 1;
    return RenderManager::StateId(next++);
}

} // namespace notf
