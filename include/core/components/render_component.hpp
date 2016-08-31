#pragma once

#include "core/component.hpp"

namespace signal {

class Widget;
class Shader;

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
    const Shader& get_shader() const {return *m_shader.get(); }

protected: // methods
    /// \brief Value Constructor.
    /// \param shader   The Shader used for rendering.
    /// \param texture  The Texture used for rendering.
    explicit RenderComponent(std::shared_ptr<Shader> shader);

protected: // fields
    /// \brief The Shader used for rendering.
    std::shared_ptr<Shader> m_shader;
};

} // namespace signal
