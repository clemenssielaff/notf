#include "graphics/vertex_object.hpp"

#include "common/exception.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/index_buffer.hpp"
#include "graphics/vertex_array.hpp"

namespace notf {

VertexObject::VertexObject(ShaderPtr shader, VertexArrayTypePtr vertices,
                           const RenderMode mode, std::unique_ptr<IndexBufferType> indices)
    : m_vao_id(0)
    , m_mode(0)
    , m_shader(std::move(shader))
    , m_vertices(std::move(vertices))
    , m_indices(std::move(indices))
{
    _set_render_mode(mode);

    glGenVertexArrays(1, &m_vao_id);
    if (!m_vao_id) {
        throw_runtime_error("Failed to alocate VertexObject");
    }

    glBindVertexArray(m_vao_id);
    m_vertices->init(m_shader);
    if (m_indices) {
        m_indices->init();
    }
}

VertexObject::~VertexObject()
{
    glDeleteVertexArrays(1, &m_vao_id);
}

void VertexObject::render() const
{
    glBindVertexArray(m_vao_id);
    if (m_indices) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices->id());
        glDrawElements(m_mode, m_indices->size(), m_indices->type(), 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else {
        glDrawArrays(m_mode, 0, m_vertices->size());
    }
    glBindVertexArray(0);
}

void VertexObject::_set_render_mode(const RenderMode mode)
{
    switch (mode) {
    case RenderMode::POINTS:
        m_mode = GL_POINTS;
        break;
    case RenderMode::LINE_STRIP:
        m_mode = GL_LINE_STRIP;
        break;
    case RenderMode::LINE_LOOP:
        m_mode = GL_LINE_LOOP;
        break;
    case RenderMode::LINES:
        m_mode = GL_LINES;
        break;
    case RenderMode::TRIANGLE_STRIP:
        m_mode = GL_TRIANGLE_STRIP;
        break;
    case RenderMode::TRIANGLE_FAN:
        m_mode = GL_TRIANGLE_FAN;
        break;
    case RenderMode::TRIANGLES:
        m_mode = GL_TRIANGLES;
        break;
    }
}

} // namespace notf
