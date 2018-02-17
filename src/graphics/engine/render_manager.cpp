#include "graphics/engine/render_manager.hpp"

#include <sstream>

#include "common/hash.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/engine/graphics_producer.hpp"
#include "graphics/text/font_manager.hpp"

namespace notf {

namespace detail {

void RenderTargetDependencies::add(const GraphicsProducerId producer, const RenderTargetId target)
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

void RenderTargetDependencies::reset() { m_new_hash = 0; }

} // namespace detail

RenderManager::RenderManager(GLFWwindow* window)
    : m_graphics_context(std::make_unique<GraphicsContext>(window))
    , m_font_manager(FontManager::create(*m_graphics_context))
    , m_dependencies()
    , m_graphics_producer()
{}

void RenderManager::register_new(GraphicsProducerPtr graphics_producer)
{
    if (m_graphics_producer.count(graphics_producer->id())) {
        notf_throw_format(runtime_error, "Failed to register GraphicsProducer with duplicate ID: \""
                                             << graphics_producer->id() << "\"");
    }
    m_graphics_producer.emplace(std::make_pair(graphics_producer->id(), graphics_producer));
}

} // namespace notf
