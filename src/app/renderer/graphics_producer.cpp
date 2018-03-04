#include "app/renderer/graphics_producer.hpp"

#include "common/exception.hpp"
#include "common/log.hpp"

namespace notf {

GraphicsProducer::~GraphicsProducer() {}

void GraphicsProducer::render() const
{
    try {
        _render();
    }
    catch (const notf_exception&) {
        log_warning << "Caught notf exception while rendering GraphicsProducer \"" << name() << "\"";
        throw;
    }
}

GraphicsProducerId GraphicsProducer::_next_id()
{
    static GraphicsProducerId::underlying_t next = 1;
    return GraphicsProducerId(next++);
}

} // namespace notf
