#include "graphics/vertex_array.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

VertexArrayType::~VertexArrayType() { glDeleteBuffers(1, &m_vbo_id); }

NOTF_CLOSE_NAMESPACE
