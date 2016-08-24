#include "core/shadercomponent.hpp"

#include <glad/glad.h>

#include "common/const.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "graphics/gl_utils.hpp"
#include "linmath.h"
#include "thirdparty/stb_image/std_image.h"

namespace { // anonymous

} // namespace anonymous

static const float transparent[] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace signal {

ShaderComponent::ShaderComponent()
    : m_vertices{
        // Positions         // Colors           // Texture Coords
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // Top Right
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Bottom Right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Bottom Left
        -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f // Top Left
    }
    , m_indices{
        0, 1, 3, // First Triangle
        1, 2, 3 // Second Triangle
    }
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_shader()
    , m_texture1()
    , m_texture2()
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

    // create the shader program
    m_shader = Shader::from_sources("../../res/shaders/test01.vert", "../../res/shaders/test01.frag");

    // define the vertex attributes
    {
        static const int vertex_dimensions = 3;
        static const int color_dimensions = 3;
        static const int texture_dimensions = 2;
        static const ulong stride = static_cast<ulong>(vertex_dimensions + color_dimensions + texture_dimensions);

        GLuint position = static_cast<GLuint>(glGetAttribLocation(m_shader.get_id(), "position"));
        glVertexAttribPointer(position, vertex_dimensions, GL_FLOAT, GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(GLfloat)),
            buffer_offset<GLfloat>(0));
        glEnableVertexAttribArray(position);

        GLuint color = static_cast<GLuint>(glGetAttribLocation(m_shader.get_id(), "color"));
        glVertexAttribPointer(color, color_dimensions, GL_FLOAT, GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(GLfloat)),
            buffer_offset<GLfloat>(vertex_dimensions));
        glEnableVertexAttribArray(color);

        GLuint tex_coord = static_cast<GLuint>(glGetAttribLocation(m_shader.get_id(), "texCoord"));
        glVertexAttribPointer(tex_coord, texture_dimensions, GL_FLOAT, GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(GLfloat)),
            buffer_offset<GLfloat>(vertex_dimensions + color_dimensions));
        glEnableVertexAttribArray(tex_coord);
    }

    // setup the textures
    m_texture1 = Texture2::load("/home/clemens/temp/container.png");
    m_texture2 = Texture2::load("/home/clemens/temp/awesomeface2.png");

    // TODO: move to window for "draw as wireframe" option?
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

ShaderComponent::~ShaderComponent()
{
    glDeleteBuffers(1, &m_vbo);
}

void ShaderComponent::update()
{
    m_shader.use();

    glActiveTexture(GL_TEXTURE0);
    m_texture1.bind();
    glUniform1i(glGetUniformLocation(m_shader.get_id(), "ourTexture1"), 0);

    glActiveTexture(GL_TEXTURE1);
    m_texture2.bind();
    glUniform1i(glGetUniformLocation(m_shader.get_id(), "ourTexture2"), 1);

    VaoBindRAII bind_vao(m_vao);
    UNUSED(bind_vao);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
}
} // namespace signal
