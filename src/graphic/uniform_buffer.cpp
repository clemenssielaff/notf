#include "notf/graphic/uniform_buffer.hpp"

#include "notf/app/resource_manager.hpp"

#include "notf/meta/log.hpp"

NOTF_USING_NAMESPACE;

// any uniform buffer =============================================================================================== //

AnyUniformBuffer::AnyUniformBuffer(std::string name) : m_name(std::move(name)) {
    NOTF_CHECK_GL(glGenBuffers(1, &m_id.data()));
    if (m_id == 0) { NOTF_THROW(OpenGLError, "Failed to create OpenGL Uniform Buffer"); }

    NOTF_LOG_TRACE("Created UniformBuffer \"{}\"", m_name);
}

AnyUniformBuffer::~AnyUniformBuffer() {
    if (!m_id.is_valid()) { return; }

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
    m_id = UniformBufferId::invalid();

    NOTF_LOG_TRACE("Destroyed UniformBuffer \"{}\"", m_name);
}

void AnyUniformBuffer::_register_ressource(AnyUniformBufferPtr uniform_buffer) {
    ResourceManager::get_instance().get_type<AnyUniformBuffer>().set(uniform_buffer->get_name(),
                                                                     std::move(uniform_buffer));
}
