#include "app/scene/layer.hpp"

#include "app/renderer/graphics_producer.hpp"
#include "app/scene/scene_manager.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "utils/make_smart_enabler.hpp"

NOTF_OPEN_NAMESPACE

Layer::Layer(SceneManagerPtr& manager, ScenePtr scene, GraphicsProducerPtr producer)
    : m_scene_manager(*manager)
    , m_scene(std::move(scene))
    , m_producer(std::move(producer))
    , m_area(Aabri::zero())
    , m_is_visible(true)
    , m_is_fullscreen(true)
{}

LayerPtr Layer::create(SceneManagerPtr& manager, ScenePtr scene, GraphicsProducerPtr producer)
{
#ifdef NOTF_DEBUG
    return LayerPtr(new Layer(manager, std::move(scene), std::move(producer)));
#else
    return std::make_shared<make_shared_enabler<Layer>>(manager, std::move(scene), std::move(producer));
#endif
}

void Layer::render()
{
    if (!m_is_visible) {
        return;
    }

    GraphicsContext& context = *m_scene_manager.graphics_context();

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

    GraphicsProducer::Access<Layer>(*m_producer).render();
}

NOTF_CLOSE_NAMESPACE
