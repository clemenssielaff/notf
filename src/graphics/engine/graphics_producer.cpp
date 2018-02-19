#include "graphics/engine/graphics_producer.hpp"

#include "common/exception.hpp"
#include "common/log.hpp"

namespace notf {

GraphicsProducer::~GraphicsProducer() {}

void GraphicsProducer::render() const
{
    if (!is_dirty()) {
        return;
    }
    try {
        _render();
    }
    catch (const notf_exception&) {
        log_warning << "Caught notf exception while rendering GraphicsProducer \"" << name() << "\"";
        throw;
    }
    m_dirtyness = DirtynessLevel::CLEAN;
}

GraphicsProducerId GraphicsProducer::_next_id()
{
    static GraphicsProducerId::underlying_t next = 1;
    return GraphicsProducerId(next++);
}

} // namespace notf
