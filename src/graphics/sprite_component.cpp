#include "graphics/sprite_component.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "core/widget.hpp"
#include "graphics/gl_utils.hpp"

namespace signal {

SpriteComponent::SpriteComponent(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2> texture)
    : RenderComponent(shader, texture)
    , m_quad(0)
{
    // Configure VAO/VBO
    GLuint vbo;


    GLfloat vertices[] = {
        // Pos      // Tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f};
    glGenVertexArrays(1, &m_quad);
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(m_quad);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), buffer_offset<GLfloat>(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_shader->use();
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(800), static_cast<GLfloat>(600), 0.0f, -1.0f, 1.0f);
    m_shader->set_uniform("image", 0);
    m_shader->set_uniform("projection", projection);
}

SpriteComponent::~SpriteComponent()
{
    glDeleteVertexArrays(1, &m_quad);
}

void SpriteComponent::render(Widget& widget)
{
    m_shader->use();

    glm::mat4 model;
    glm::vec2 position (0, 0);
    GLfloat rotate = 0.0f;
    glm::vec2 size (800, 600);
    glm::vec3 color (1.0f);

    model = glm::translate(model, glm::vec3(position, 0.0f)); // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    m_shader->set_uniform("model", model);

    // Render textured quad
    m_shader->set_uniform("spriteColor", color);

    glActiveTexture(GL_TEXTURE0);
    m_texture->bind();

    glBindVertexArray(m_quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace signal
