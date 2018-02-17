#include "graphics/engine/graphics_producer.hpp"

namespace notf {

GraphicsProducer::~GraphicsProducer() {}

GraphicsProducerId GraphicsProducer::_next_id()
{
    static GraphicsProducerId::underlying_t next = 1;
    return GraphicsProducerId(next++);
}

} // namespace notf
