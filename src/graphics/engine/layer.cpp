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
    , m_size(Size2i::zero())
    , m_position(Vector2s::zero())
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
        context.set_render_size(context.window_size());
    }
    else {
        if (m_size.is_zero()) {
            return;
        }
        if (!m_size.is_valid()) {
            log_warning << "Cannot render a Layer with an invalid size";
            return;
        }
        context.set_render_size(m_size);
    }

    m_producer->set_dirty(); // force the producer to render
    GraphicsProducer::Private<Layer>(*m_producer).render();

    // TODO: render attribute-less square here .. or not? WTF?
    // ... but wait ... the Layer doesn't actually render antyhing itself ... so how does it
    // manage the producers size and position?
}

} // namespace notf
