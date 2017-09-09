#include "graphics/vertex_object.hpp"

namespace notf {

IndexBufferType::~IndexBufferType()
{
    glDeleteBuffers(1, &m_vbo_id);
}

} // namespace notf
