#include "app/layer.hpp"

#include "app/renderer.hpp"
#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"

NOTF_OPEN_NAMESPACE

Layer::Layer(Window& window, RendererPtr renderer, ScenePtr& scene)
    : m_window(window), m_scene(std::move(scene)), m_renderer(std::move(renderer))
{}

LayerPtr Layer::create(Window& window, RendererPtr renderer, ScenePtr& scene)
{
    return NOTF_MAKE_UNIQUE_FROM_PRIVATE(Layer, window, std::move(renderer), scene);
}

Layer::~Layer() = default;

void Layer::render()
{
    if (!m_is_visible) {
        return;
    }

    GraphicsContext& context = m_window.graphics_context();

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

    Renderer::Access<Layer>(*m_renderer).render();
}

NOTF_CLOSE_NAMESPACE
