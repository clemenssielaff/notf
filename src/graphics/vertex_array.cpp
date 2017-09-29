#include "graphics/vertex_array.hpp"

#include "common/log.hpp"

namespace notf {

VertexArrayType::~VertexArrayType()
{
    glDeleteBuffers(1, &m_vbo_id);
}

GLuint VertexArrayType::_get_shader_attribute(const Shader* shader, const std::string attribute_name)
{
    try {
        return shader->attribute(attribute_name);
    } catch (const std::runtime_error&) {
        log_warning << "Ignoring unknown attribute: \"" << attribute_name << "\"";
        return 0;
    }
}

} // namespace notf
