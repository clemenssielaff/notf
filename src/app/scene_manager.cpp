#include "app/scene_manager.hpp"

#include <sstream>

#include "app/graphics_producer.hpp"
#include "app/io/char_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/io/window_event.hpp"
#include "app/layer.hpp"
#include "app/render_target.hpp"
#include "app/scene.hpp"
#include "common/hash.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
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

//====================================================================================================================//

const SceneManager::State SceneManager::s_default_state = {};

SceneManager::SceneManager() : m_state(&s_default_state), m_dependencies(), m_graphics_producer(), m_render_targets() {}

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

void SceneManager::propagate(MouseEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void SceneManager::propagate(KeyEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void SceneManager::propagate(CharEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void SceneManager::propagate(WindowEvent&& event)
{
    assert(!event.was_handled());
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->propagate(event);
        if (event.was_handled()) {
            return;
        }
    }
}

void SceneManager::resize(Size2i size)
{
    for (const LayerPtr& layer : m_state->layers) {
        layer->scene()->resize(size);
    }
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
