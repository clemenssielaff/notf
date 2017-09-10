#include "graphics/index_buffer.hpp"

namespace notf {

IndexBufferType::~IndexBufferType()
{
    glDeleteBuffers(1, &m_vbo_id);
}

} // namespace notf
