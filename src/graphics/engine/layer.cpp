#include "graphics/engine/layer.hpp"

#include "graphics/engine/graphics_producer.hpp"

namespace notf {

Layer::Layer(GraphicsProducerPtr producer)
    : m_producer(std::move(producer))
    , m_size(Size2i::zero())
    , m_position(Vector2s::zero())
    , m_is_visible(true)
    , m_is_fullscreen(true)
{}

void Layer::render()
{
    m_producer->set_dirty(); // force the producer to render
    m_producer->render();
}

} // namespace notf
