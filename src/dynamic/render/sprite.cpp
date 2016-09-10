#include "dynamic/render/sprite.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "common/size2.hpp"
#include "core/components/texture_component.hpp"
#include "core/widget.hpp"
#include "core/window.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"

namespace { // anonymous

const char* image_var = "image";
const char* projection_var = "projection";
const char* model_var = "model";
const char* color_var = "sprite_color";

} // anonymous

namespace signal {

SpriteRenderer::SpriteRenderer(std::shared_ptr<Shader> shader)
    : RenderComponent(shader)
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
}

bool SpriteRenderer::is_valid()
{
    return RenderComponent::is_valid()
        && assert_uniform(image_var)
        && assert_uniform(projection_var)
        && assert_uniform(model_var)
        && assert_uniform(color_var);
}

SpriteRenderer::~SpriteRenderer()
{
    glDeleteVertexArrays(1, &m_quad);
}

void SpriteRenderer::setup_window(const Window& window)
{
    Size2 size = window.get_canvas_size();
    glm::mat4 projection_matrix = glm::ortho(0.0f, static_cast<GLfloat>(size.width), static_cast<GLfloat>(size.height), 0.0f, -1.0f, 1.0f);
    m_shader->set_uniform(image_var, 0);
    m_shader->set_uniform(projection_var, projection_matrix);
}

void SpriteRenderer::render(const Widget& widget)
{
    Size2 canvas_size = widget.get_window()->get_canvas_size();

    glm::mat4 model;
    glm::vec2 position(0, 0);
    GLfloat rotate = 0.0f;
    glm::vec2 size(canvas_size.width, canvas_size.height);
    glm::vec3 color(1.0f);

    model = glm::translate(model, glm::vec3(position, 0.0f)); // First translate (transformations are: scale happens first, then rotation and then finall translation happens; reversed order)

    model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // Move origin of rotation to center of quad
    model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f)); // Then rotate
    model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // Move origin back

    model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

    m_shader->set_uniform(model_var, model);

    // Render textured quad
    m_shader->set_uniform(color_var, color);

    m_shader->use();

    if (auto texture_component = widget.get_component<TextureComponent>()) {
        texture_component->bind_texture(0);
    }

    glBindVertexArray(m_quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace signal
