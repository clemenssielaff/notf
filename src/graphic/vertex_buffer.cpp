#include "notf/graphic/vertex_buffer.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/meta/log.hpp"

NOTF_USING_NAMESPACE;

// any vertex array ================================================================================================= //

AnyVertexBuffer::AnyVertexBuffer(std::string name, Args args) : m_args(std::move(args)), m_name(std::move(name)) {
    NOTF_CHECK_GL(glGenBuffers(1, &m_id.data()));
    if (!m_id) { NOTF_THROW(OpenGLError, "Failed to generate an OpenGL buffer for VertexBuffer \"{}\"", m_name); }
    NOTF_LOG_TRACE("Created VertexBuffer \"{}\"", m_name);
}

AnyVertexBuffer::~AnyVertexBuffer() {
    if (!m_id.is_valid()) { return; }

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
    m_id = VertexBufferId::invalid();

    NOTF_LOG_TRACE("Destroyed VertexBuffer \"{}\"", m_name);
}

void AnyVertexBuffer::_register_ressource(AnyVertexBufferPtr vertex_buffer) {
    ResourceManager::get_instance().get_type<AnyVertexBuffer>().set(vertex_buffer->get_name(),
                                                                    std::move(vertex_buffer));
}
