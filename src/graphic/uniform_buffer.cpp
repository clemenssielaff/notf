#include "notf/graphic/uniform_buffer.hpp"

#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

// any uniform buffer =============================================================================================== //

namespace detail {

AnyUniformBuffer::AnyUniformBuffer(std::string name) : m_name(std::move(name)) {
    NOTF_CHECK_GL(glGenBuffers(1, &m_id.data()));
    if (m_id == 0) { NOTF_THROW(OpenGLError, "Failed to create OpenGL Uniform Buffer"); }
}

AnyUniformBuffer::~AnyUniformBuffer() {
    if (m_id != 0) {
        NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
        m_id = 0;
    }
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
