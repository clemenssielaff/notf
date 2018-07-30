#include "graphics/index_array.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

IndexArrayType::~IndexArrayType() { notf_check_gl(glDeleteBuffers(1, &m_vbo_id)); }

NOTF_CLOSE_NAMESPACE
