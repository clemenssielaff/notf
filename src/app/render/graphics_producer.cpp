#include "app/render/graphics_producer.hpp"

#include "common/exception.hpp"
#include "common/log.hpp"

NOTF_OPEN_NAMESPACE

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

NOTF_CLOSE_NAMESPACE
