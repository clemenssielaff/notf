#pragma once

#include "core/components/render_component.hpp"
#include "graphics/gl_forwards.hpp"

namespace signal {

class SpriteRenderer : public RenderComponent {

public: // methods
    /// \brief Destructor
    virtual ~SpriteRenderer() override;

    /// \brief Checks if the given Shader fits this Component.
    /// Is called in the constructor and reports any errors to the console.
    /// If the passed Shader is not compatible, the resulting RenderComponent will be invalid.
    virtual bool is_valid() override;

    // \brief Configures the renderer to render to the given Window.
    // \param window    Window to render to.
    virtual void setup_window(const Window& window) override;

    /// \brief Renders the given Widget.
    /// \param widget   Widget to render.
    virtual void render(const Widget &widget) override;

protected: // methods
    /// \brief Value Constructor.
    /// \param shader   The Shader used for rendering.
    /// \param texture  The Texture used for rendering.
    explicit SpriteRenderer(std::shared_ptr<Shader> shader);

private: // fields
    /// \brief Vertex Array Object of the rendered quat.
    GLuint m_quad;
};

} // namespace signal
