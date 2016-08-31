#pragma once

#include "core/component.hpp"
#include "graphics/gl_forwards.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture2.hpp"

namespace signal {

class Widget;

/// \brief Virtual base class for all Render Components.
///
class RenderComponent : public Component {

public: // methods
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::RENDER; }

    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    virtual void render(Widget& widget) = 0;

    /// \brief OpenGL ID of the shader used for rendering.
    GLuint get_shader_id() const { return m_shader->get_id(); }

    /// \brief OpenGL ID of the texture used for rendering.
    GLuint get_texture_id() const { return m_texture->get_id(); }

protected: // methods
    /// \brief Value Constructor.
    /// \param shader   The Shader used for rendering.
    /// \param texture  The Texture used for rendering.
    explicit RenderComponent(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2> texture);

protected: // fields
    /// \brief The Shader used for rendering.
    std::shared_ptr<Shader> m_shader;

    /// \brief The Texture used for rendering.
    std::shared_ptr<Texture2> m_texture;
};

} // namespace signal
