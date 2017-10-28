#include "graphics/index_array.hpp"

namespace notf {

IndexArrayType::~IndexArrayType() { glDeleteBuffers(1, &m_vbo_id); }

} // namespace notf
