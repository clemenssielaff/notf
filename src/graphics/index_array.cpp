#include "graphics/index_array.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

IndexArrayType::~IndexArrayType() { NOTF_CHECK_GL(glDeleteBuffers(1, &m_vbo_id)); }

NOTF_CLOSE_NAMESPACE
