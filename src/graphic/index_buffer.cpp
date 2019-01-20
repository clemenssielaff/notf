#include "notf/graphic/index_buffer.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/meta/log.hpp"

NOTF_USING_NAMESPACE;

// any index buffer ================================================================================================= //

AnyIndexBuffer::AnyIndexBuffer(std::string name, Args args) : m_args(std::move(args)), m_name(std::move(name)) {
    NOTF_CHECK_GL(glGenBuffers(1, &m_id.data()));
    if (!m_id) { NOTF_THROW(OpenGLError, "Failed to generate an OpenGL buffer for IndexBuffer \"{}\"", m_name); }
    NOTF_LOG_TRACE("Created IndexBuffer \"{}\"", m_name);
}

AnyIndexBuffer::~AnyIndexBuffer() {
    if (!m_id.is_valid()) { return; }

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
    m_id = IndexBufferId::invalid();

    NOTF_LOG_TRACE("Destroyed IndexBuffer \"{}\"", m_name);
}

void AnyIndexBuffer::_register_ressource(AnyIndexBufferPtr index_buffer) {
    ResourceManager::get_instance().get_type<AnyIndexBuffer>().set(index_buffer->get_name(), std::move(index_buffer));
}
