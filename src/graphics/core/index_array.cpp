#include "graphics/core/index_array.hpp"

namespace notf {

IndexArrayType::~IndexArrayType() { gl_check(glDeleteBuffers(1, &m_vbo_id)); }

} // namespace notf
