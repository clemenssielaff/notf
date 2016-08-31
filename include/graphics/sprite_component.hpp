#pragma once

#include "core/components/render_component.hpp"

namespace signal {

class SpriteComponent : public RenderComponent {

public: // methods
    /// \brief Destructor
    virtual ~SpriteComponent() override;

    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    ///
    virtual void render(Widget& widget) override;

protected: // methods
    /// \brief Value Constructor.
    /// \param shader   The Shader used for rendering.
    /// \param texture  The Texture used for rendering.
    explicit SpriteComponent(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2> texture);

private: // fields
    /// \brief Vertex Array Object of the rendered quat.
    GLuint m_quad;
};

} // namespace signal
