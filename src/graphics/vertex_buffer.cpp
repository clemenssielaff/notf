#include "graphics/vertex_buffer.hpp"

namespace notf {

VertexBufferType::~VertexBufferType()
{
    glDeleteBuffers(1, &m_vbo_id);
}

} // namespace notf
