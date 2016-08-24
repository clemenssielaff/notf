#pragma once

#include <vector>

#include "core/component.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"

namespace signal {

class ShaderComponent : public Component {

protected:
    explicit ShaderComponent();

public: // methods
    /// \brief Destructor.
    virtual ~ShaderComponent() override;

    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::TEXTURE; }

    /// \brief Updates this Component.
    virtual void update() override;

private: // fields
    const std::vector<GLfloat> m_vertices;

    const std::vector<GLuint> m_indices;

    GLuint m_vao;

    GLuint m_vbo;

    GLuint m_ebo;

    Shader m_shader;

    Texture2 m_texture1;
    Texture2 m_texture2;
};

} // namespace signal
