#include "core/shadercomponent.hpp"

#include "common/const.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "core/glfw_wrapper.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/load_shaders.hpp"
#include "linmath.h"
#include "thirdparty/stb_image/std_image.h"

namespace { // anonymous

} // namespace anonymous

static const float transparent[] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace signal {

ShaderComponent::ShaderComponent()
    : m_vertices{
        // Positions         // Colors           // Texture Coords
        0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // Top Right
        0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // Bottom Right
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // Bottom Left
        -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // Top Left
    }
    , m_indices{
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    }
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_program(0)
    , m_texture1(0)
    , m_texture2(0)
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
    m_program = produce_gl_program( // TODO: own class for Shader programs? (that call glDeleteProgram themselves)
        "../../res/shaders/test01.vert", // TODO: what about other wrapper classes for VBOs?
        "../../res/shaders/test01.frag");

    // define the vertex attributes
    {
        static const int vertex_dimensions = 3;
        static const int color_dimensions = 3;
        static const int texture_dimensions = 2;
        static const ulong stride = static_cast<ulong>(vertex_dimensions + color_dimensions + texture_dimensions);

        GLuint position = static_cast<GLuint>(glGetAttribLocation(m_program, "position"));
        glVertexAttribPointer(position, vertex_dimensions, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride * sizeof(GLfloat)),
                              buffer_offset<GLfloat>(0));
        glEnableVertexAttribArray(position);

        GLuint color = static_cast<GLuint>(glGetAttribLocation(m_program, "color"));
        glVertexAttribPointer(color, color_dimensions, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride * sizeof(GLfloat)),
                              buffer_offset<GLfloat>(vertex_dimensions));
        glEnableVertexAttribArray(color);

        GLuint tex_coord = static_cast<GLuint>(glGetAttribLocation(m_program, "texCoord"));
        glVertexAttribPointer(tex_coord, texture_dimensions, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride * sizeof(GLfloat)),
                              buffer_offset<GLfloat>(vertex_dimensions + color_dimensions));
        glEnableVertexAttribArray(tex_coord);
    }

    // setup the textures
    {
        int tex_width, tex_height, tex_bits;
        static std::string texture_path = "/home/clemens/temp/container.png";
        unsigned char *data = stbi_load(texture_path.c_str(), &tex_width, &tex_height, &tex_bits, 0);
        if(!data){
            log_critical << "Failed to load file from: " << texture_path;
        } else {
            glGenTextures(1, &m_texture1);
            glBindTexture(GL_TEXTURE_2D, m_texture1);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, transparent);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        stbi_image_free(data);
    }
    {
        int tex_width, tex_height, tex_bits;
        static std::string texture_path = "/home/clemens/temp/awesomeface.jpg";
        unsigned char *data = stbi_load(texture_path.c_str(), &tex_width, &tex_height, &tex_bits, 0);
        if(!data){
            log_critical << "Failed to load file from: " << texture_path;
        } else {
            glGenTextures(1, &m_texture2);
            glBindTexture(GL_TEXTURE_2D, m_texture2);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, transparent);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        stbi_image_free(data);
    }

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture1);
    glUniform1i(glGetUniformLocation(m_program, "ourTexture1"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture2);
    glUniform1i(glGetUniformLocation(m_program, "ourTexture2"), 1);

    VaoBindRAII bind_vao(m_vao);
    UNUSED(bind_vao);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
}
} // namespace signal
