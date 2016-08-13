#include "core/shadercomponent.hpp"

#include "common/const.hpp"
#include "common/random.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/load_shaders.hpp"
#include "linmath.h"

namespace signal {

ShaderComponent::ShaderComponent()
    : m_vertices{
        0.5f, 0.5f, 0.0f, // Top Right
        0.5f, -0.5f, 0.0f, // Bottom Right
        -0.5f, -0.5f, 0.0f, // Bottom Left
        -0.5f, 0.5f, 0.0f // Top Left
    }
    , m_indices{
        0, 1, 3, // First Triangle
        1, 2, 3 // Second Triangle
    }
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_program(0)
{
    // create and bind the vertex array object (VAO)
    glGenVertexArrays(1, &m_vao);
    VaoBindRAII bind_vao(m_vao);
    UNUSED(bind_vao);

    // create the vertex buffer
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(decltype(m_vertices)::value_type)),
        m_vertices.data(),
        GL_STATIC_DRAW);

    // create the element buffer
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_indices.size() * sizeof(decltype(m_indices)::value_type)),
        m_indices.data(),
        GL_STATIC_DRAW);

    // define the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), buffer_offset(0));
    glEnableVertexAttribArray(0);

    // create the shader program
    m_program = produce_gl_program(
        "../../res/shaders/test01.vert",
        "../../res/shaders/test01.frag");

    // TODO: move to window for "draw as wireframe" option?
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

ShaderComponent::~ShaderComponent()
{
    glDeleteBuffers(1, &m_vbo);
    glDeleteProgram(m_program); // ignored if uninitialized
}

void ShaderComponent::update()
{
    glUseProgram(m_program);
    VaoBindRAII bind_vao(m_vao);
    UNUSED(bind_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
}

} // namespace signal
