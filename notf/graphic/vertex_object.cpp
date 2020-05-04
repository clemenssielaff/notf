#include "notf/graphic/vertex_object.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/vertex_buffer.hpp"

NOTF_USING_NAMESPACE;

// vertex object ==================================================================================================== //

VertexObject::VertexObject(GraphicsContext& context, std::string name) : m_context(context), m_name(std::move(name)) {
    // generate VAO
    NOTF_CHECK_GL(glGenVertexArrays(1, &m_id.data()));
    if (!m_id.is_valid()) { NOTF_THROW(OpenGLError, "Failed to generate VAO for VertexObject \"{}\"", m_name); }
    NOTF_LOG_TRACE("Generate VAO for VertexObject \"{}\" with id {}", m_name, m_id);
}

VertexObjectPtr VertexObject::create(GraphicsContext& context, std::string name) {
    VertexObjectPtr vertex_object
        = _create_shared(context, std::move(name));
    GraphicsContext::AccessFor<VertexObject>::register_new(context, vertex_object);
    return vertex_object;
}

void VertexObject::_deallocate() {
    if (!m_id.is_valid()) { return; }

    NOTF_GUARD(m_context.make_current());

    m_index_buffer.reset();
    m_vertex_buffers.clear();

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
    m_id = VertexObjectId::invalid();

    NOTF_LOG_TRACE("Destroyed VAO of VertexObject \"{}\"", m_name);
}
