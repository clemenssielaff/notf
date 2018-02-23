#include "graphics/engine/layer.hpp"

#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/engine/graphics_producer.hpp"
#include "graphics/engine/render_manager.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

Layer::Layer(RenderManagerPtr& manager, GraphicsProducerPtr producer)
    : m_render_manager(*manager)
    , m_producer(std::move(producer))
    , m_area(Aabri::zero())
    , m_is_visible(true)
    , m_is_fullscreen(true)
{}

LayerPtr Layer::create(RenderManagerPtr& manager, GraphicsProducerPtr producer)
{
#ifdef NOTF_DEBUG
    return LayerPtr(new Layer(manager, std::move(producer)));
#else
    return std::make_shared<make_shared_enabler<Layer>>(manager, std::move(producer));
#endif
}

void Layer::render()
{
    if (!m_is_visible) {
        return;
    }

    GraphicsContext& context = *m_render_manager.graphics_context();

    if (m_is_fullscreen) {
        context.set_render_area(context.window_size());
    }
    else {
        if (m_area.is_zero()) {
            return;
        }
        if (!m_area.is_valid()) {
            log_warning << "Cannot render a Layer with an invalid area";
            return;
        }
        context.set_render_area(m_area);
    }

    // TODO: are producers dirty? I would guess that RenderTargets are dirty, not producers...
    m_producer->set_dirty(); // force the producer to render
    GraphicsProducer::Private<Layer>(*m_producer).render();
}

} // namespace notf
