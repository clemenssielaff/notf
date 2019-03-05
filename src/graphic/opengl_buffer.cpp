#include "notf/graphic/opengl_buffer.hpp"

#include "notf/meta/log.hpp"

NOTF_USING_NAMESPACE;

// any opengl buffer ================================================================================================ //

namespace notf::detail {

AnyOpenGLBuffer::AnyOpenGLBuffer(std::string name, const UsageHint usage_hint, const Type buffer_type)
    : m_name(std::move(name)), m_usage(usage_hint), m_type(buffer_type) {

    NOTF_CHECK_GL(glGenBuffers(1, &m_handle));

    const char* type_name = _to_type_name(m_type);
    if (!m_handle) { NOTF_THROW(OpenGLError, "Failed to generate an OpenGL buffer for {} \"{}\"", type_name, m_name); }
    NOTF_LOG_TRACE("Created {} \"{}\"", type_name, m_name);
}

AnyOpenGLBuffer::~AnyOpenGLBuffer() {
    if (!m_handle) { return; }

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_handle));
    m_handle = 0;

    const char* type_name = _to_type_name(m_type);
    NOTF_LOG_TRACE("Destroyed OpenGLBuffer of {} \"{}\"", type_name, m_name);
}

const char* AnyOpenGLBuffer::_to_type_name(const Type buffer_type) {
    switch (buffer_type) {
    case Type::VERTEX: return "VertexBuffer";
    case Type::INDEX: return "IndexBuffer";
    case Type::UNIFORM: return "UniformBuffer";
    case Type::DRAWCALL: return "DrawCall";
    }
}

GLenum AnyOpenGLBuffer::_to_gl_usage(const UsageHint usage) {
    switch (usage) {
    case UsageHint::DYNAMIC_DRAW: return GL_DYNAMIC_DRAW;
    case UsageHint::DYNAMIC_READ: return GL_DYNAMIC_READ;
    case UsageHint::DYNAMIC_COPY: return GL_DYNAMIC_COPY;
    case UsageHint::STATIC_DRAW: return GL_STATIC_DRAW;
    case UsageHint::STATIC_READ: return GL_STATIC_READ;
    case UsageHint::STATIC_COPY: return GL_STATIC_COPY;
    case UsageHint::STREAM_DRAW: return GL_STREAM_DRAW;
    case UsageHint::STREAM_READ: return GL_STREAM_READ;
    case UsageHint::STREAM_COPY: return GL_STREAM_COPY;
    }
}

void AnyOpenGLBuffer::_log_buffer_size(const size_t size) const {
    NOTF_LOG_TRACE("Resizing OpenGLBuffer \"{}\" to {} bytes", m_name, size);
}

} // namespace notf::detail
