#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/gl_forwards.hpp"

namespace signal {
class Texture2;
class Shader;
}
using namespace signal;

class SpriteRenderer {
public:
    explicit SpriteRenderer(std::shared_ptr<Shader> shader);

    ~SpriteRenderer();

    void drawSprite(Texture2* texture, glm::vec2 position,
                    glm::vec2 size = glm::vec2(10, 10), GLfloat rotate = 0.0f, glm::vec3 color = glm::vec3(1.0f));

private: // fields
    std::shared_ptr<Shader> m_shader;

    GLuint m_quadVAO;
};
