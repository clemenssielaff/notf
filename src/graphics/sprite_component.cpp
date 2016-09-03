#include "graphics/sprite_component.hpp"

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

/**
 * \brief Convenience function to translate a Texture channel into an OpenGL GL_TEXTURE* format.
 * \param channel   The channel to translate.
 * \return The corresponding GL_TEXTURE* value.
 */
constexpr GLenum gl_texture_channel(ushort channel)
{
    switch (channel) {
    case (0): return GL_TEXTURE0;
    case (1): return GL_TEXTURE1;
    case (2): return GL_TEXTURE2;
    case (3): return GL_TEXTURE3;
    case (4): return GL_TEXTURE4;
    case (5): return GL_TEXTURE5;
    case (6): return GL_TEXTURE6;
    case (7): return GL_TEXTURE7;
    case (8): return GL_TEXTURE8;
    case (9): return GL_TEXTURE9;
    case (10): return GL_TEXTURE10;
    case (11): return GL_TEXTURE11;
    case (12): return GL_TEXTURE12;
    case (13): return GL_TEXTURE13;
    case (14): return GL_TEXTURE14;
    case (15): return GL_TEXTURE15;
    case (16): return GL_TEXTURE16;
    case (17): return GL_TEXTURE17;
    case (18): return GL_TEXTURE18;
    case (19): return GL_TEXTURE19;
    case (20): return GL_TEXTURE20;
    case (21): return GL_TEXTURE21;
    case (22): return GL_TEXTURE22;
    case (23): return GL_TEXTURE23;
    case (24): return GL_TEXTURE24;
    case (25): return GL_TEXTURE25;
    case (26): return GL_TEXTURE26;
    case (27): return GL_TEXTURE27;
    case (28): return GL_TEXTURE28;
    case (29): return GL_TEXTURE29;
    case (30): return GL_TEXTURE30;
    case (31): return GL_TEXTURE31;
    default: return GL_ACTIVE_TEXTURE;
    }
}

const char* image_var = "image";
const char* projection_var = "projection";
const char* model_var = "model";
const char* color_var = "sprite_color";

} // anonymous

namespace signal {

SpriteComponent::SpriteComponent(std::shared_ptr<Shader> shader)
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

bool SpriteComponent::is_valid()
{
    return RenderComponent::is_valid()
        && assert_uniform(image_var)
        && assert_uniform(projection_var)
        && assert_uniform(model_var)
        && assert_uniform(color_var);
}

SpriteComponent::~SpriteComponent()
{
    glDeleteVertexArrays(1, &m_quad);
}

void SpriteComponent::setup_window(const Window& window)
{
    Size2 size = window.get_canvas_size();
    glm::mat4 projection_matrix = glm::ortho(0.0f, static_cast<GLfloat>(size.width), static_cast<GLfloat>(size.height), 0.0f, -1.0f, 1.0f);
    m_shader->set_uniform(image_var, 0);
    m_shader->set_uniform(projection_var, projection_matrix);
}

void SpriteComponent::render(const Widget& widget)
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

    // TODO: that should be part of the TExtureComponent (see Trello TODO)

    if (std::shared_ptr<TextureComponent> texture_component = std::static_pointer_cast<TextureComponent>(widget.get_component(KIND::TEXTURE))) {
        for (auto it = texture_component->all_textures().cbegin(); it != texture_component->all_textures().cend(); ++it) {
            glActiveTexture(gl_texture_channel(it->first));
            it->second->bind();
        }
    }

    glBindVertexArray(m_quad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace signal
