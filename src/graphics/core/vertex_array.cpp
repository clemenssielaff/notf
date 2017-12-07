#include "graphics/core/vertex_array.hpp"

namespace notf {

VertexArrayType::~VertexArrayType() { glDeleteBuffers(1, &m_vbo_id); }

} // namespace notf
