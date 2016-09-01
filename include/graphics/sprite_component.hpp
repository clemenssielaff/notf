#pragma once

#include "core/components/render_component.hpp"
#include "graphics/gl_forwards.hpp"

namespace signal {

class SpriteComponent : public RenderComponent {

public: // methods
    /// \brief Destructor
    virtual ~SpriteComponent() override;

    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    virtual void render(const Widget &widget) override;

protected: // methods
    /// \brief Value Constructor.
    /// \param shader   The Shader used for rendering.
    /// \param texture  The Texture used for rendering.
    explicit SpriteComponent(std::shared_ptr<Shader> shader);

private: // fields
    /// \brief Vertex Array Object of the rendered quat.
    GLuint m_quad;
};

} // namespace signal
