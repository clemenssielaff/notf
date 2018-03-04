#include "app/scene/scene_manager.hpp"

#include <sstream>

#include "app/renderer/graphics_producer.hpp"
#include "app/renderer/render_target.hpp"
#include "app/scene/layer.hpp"
#include "common/hash.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/text/font_manager.hpp"
#include "utils/make_smart_enabler.hpp"

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

const SceneManager::State SceneManager::s_default_state = {};

SceneManager::SceneManager(GLFWwindow* window)
    : m_graphics_context(std::make_unique<GraphicsContext>(window))
    , m_font_manager(FontManager::create(*m_graphics_context))
    , m_dependencies()
    , m_graphics_producer()
    , m_render_targets()
    , m_state(&s_default_state)
{}

SceneManagerPtr SceneManager::create(GLFWwindow* window)
{
#ifdef NOTF_DEBUG
    return SceneManagerPtr(new SceneManager(window));
#else
    return std::make_unique<make_shared_enabler<SceneManager>>(window);
#endif
}

SceneManager::~SceneManager() = default;

SceneManager::StateId SceneManager::add_state(State&& state)
{
    StateId new_id = _next_id();
    m_states.emplace(std::make_pair(new_id, std::move(state)));
    return new_id;
}

const SceneManager::State& SceneManager::state(const StateId id) const
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "SceneManager has no State with the ID \"" << id << "\"");
    }
    return it->second;
}

void SceneManager::enter_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "SceneManager has no State with the ID \"" << id << "\"");
    }
    m_state = &it->second;
}

void SceneManager::remove_state(const StateId id)
{
    auto it = m_states.find(id);
    if (it == m_states.end()) {
        notf_throw_format(resource_error, "SceneManager has no State with the ID \"" << id << "\"");
    }
    if (m_state == &it->second) {
        log_warning << "Removing current SceneManager state \"" << it->first
                    << "\" - falling back to the default state";
        m_state = &s_default_state;
    }
    m_states.erase(it);
}

void SceneManager::render()
{
    if (m_state == &s_default_state) {
        log_trace << "Ignoring SceneManager::render with the default State";
        return;
    }

    // TODO: clean all the render targets here
    // in order to sort them, use typeid(*ptr).hash_code()

    m_graphics_context->begin_frame();

    // render all Layers
    for (const LayerPtr& layer : m_state->layers) {
        layer->render();
    }

    m_graphics_context->finish_frame();
}

void SceneManager::_register_new(GraphicsProducerPtr graphics_producer)
{
    if (m_graphics_producer.count(graphics_producer->id())) {
        notf_throw_format(runtime_error, "Failed to register GraphicsProducer with duplicate ID: \""
                                             << graphics_producer->id() << "\"");
    }
    m_graphics_producer.emplace(std::make_pair(graphics_producer->id(), graphics_producer));
}

void SceneManager::_register_new(RenderTargetPtr render_target)
{
    if (m_render_targets.count(render_target->id())) {
        notf_throw_format(runtime_error,
                          "Failed to register RenderTarget with duplicate ID: \"" << render_target->id() << "\"");
    }
    m_render_targets.emplace(std::make_pair(render_target->id(), render_target));
}

SceneManager::StateId SceneManager::_next_id()
{
    static SceneManager::StateId::underlying_t next = 1;
    return SceneManager::StateId(next++);
}

NOTF_CLOSE_NAMESPACE
