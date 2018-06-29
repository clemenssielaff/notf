#include "app/layer.hpp"

#include "app/scene.hpp"
#include "app/window.hpp"
#include "common/log.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/renderer/renderer.hpp"

NOTF_OPEN_NAMESPACE

Layer::Layer(Window& window, valid_ptr<RendererPtr> renderer, valid_ptr<ScenePtr> scene)
    : m_window(window), m_scene(std::move(scene)), m_renderer(std::move(renderer))
{}

LayerPtr Layer::create(Window& window, valid_ptr<RendererPtr> renderer, valid_ptr<ScenePtr> scene)
{
    return NOTF_MAKE_SHARED_FROM_PRIVATE(Layer, window, std::move(renderer), std::move(scene));
}

Layer::~Layer() = default;

void Layer::render()
{
    if (!is_visible()) {
        return;
    }

    // define render area
    GraphicsContext& context = m_window.get_graphics_context();
    if (m_is_fullscreen) {
        context.set_render_area(context.get_window_size());
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

    Renderer::Access<Layer>::render(*m_renderer, raw_pointer(m_scene));
}

NOTF_CLOSE_NAMESPACE
