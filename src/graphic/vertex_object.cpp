#include "notf/graphic/vertex_object.hpp"

#include "notf/meta/log.hpp"

#include "notf/graphic/graphics_context.hpp"
#include "notf/graphic/vertex_buffer.hpp"

NOTF_USING_NAMESPACE;

// vertex object ==================================================================================================== //

VertexObject::VertexObject(GraphicsContext& context, std::string name, AnyVertexBufferPtr vertex_buffer,
                           AnyIndexBufferPtr index_buffer)
    : m_context(context)
    , m_name(std::move(name))
    , m_vertex_buffer(std::move(vertex_buffer))
    , m_index_buffer(std::move(index_buffer)) {

    // check arguments
    if (!m_vertex_buffer) { NOTF_THROW(ValueError, "Cannot construct a VertexObject with an invalid VertexBuffer"); }
    if (!m_index_buffer) { NOTF_THROW(ValueError, "Cannot construct a VertexObject with an invalid IndexBuffer"); }

    // generate VAO
    NOTF_CHECK_GL(glGenVertexArrays(1, &m_id.data()));
    if (!m_id.is_valid()) { NOTF_THROW(OpenGLError, "Failed to generate VAO for VertexObject \"{}\"", m_name); }
    NOTF_LOG_TRACE("Generate VAO for VertexObject \"{}\" with id {}", m_name, m_id);

    // bind and eventually unbind the vao
    struct VaoGuard {
        VaoGuard(const GLuint vao_id) { NOTF_CHECK_GL(glBindVertexArray(vao_id)); }
        ~VaoGuard() { NOTF_CHECK_GL(glBindVertexArray(0)); }
    };
    NOTF_GUARD(VaoGuard(m_id.get_value()));

    m_vertex_buffer->initialize();
    m_index_buffer->initialize();
}

VertexObjectPtr VertexObject::create(GraphicsContext& context, std::string name, AnyVertexBufferPtr vertex_buffer,
                                     AnyIndexBufferPtr index_buffer) {
    VertexObjectPtr vertex_object
        = _create_shared(context, std::move(name), std::move(vertex_buffer), std::move(index_buffer));
    GraphicsContext::AccessFor<VertexObject>::register_new(context, vertex_object);
    return vertex_object;
}

void VertexObject::_deallocate() {
    if (!m_id.is_valid()) { return; }

    NOTF_GUARD(m_context.make_current());

    m_index_buffer.reset();
    m_vertex_buffer.reset();

    NOTF_CHECK_GL(glDeleteBuffers(1, &m_id.data()));
    m_id = VertexObjectId::invalid();

    NOTF_LOG_TRACE("Destroyed VAO of VertexObject \"{}\"", m_name);
}