#include "graphics/core/index_array.hpp"

NOTF_OPEN_NAMESPACE

IndexArrayType::~IndexArrayType() { gl_check(glDeleteBuffers(1, &m_vbo_id)); }

NOTF_CLOSE_NAMESPACE
