#pragma once

#include "core/component.hpp"

namespace notf {

class Widget;
class Window;
class Shader;

/// @brief Virtual base class for all Render Components.
///
class RenderComponent : public Component {

public: // methods
    /// @brief Checks if the given Shader is valid.
    virtual bool is_valid() override;

    /// @brief This Component's type.
    virtual KIND get_kind() const override { return KIND::RENDER; }

    /// @brief OpenGL ID of the shader used for rendering, is empty if invalid.
    Shader& get_shader() { return *m_shader.get(); }

    // @brief Configures the renderer to render to the given Window.
    // @param window    Window to render to.
    virtual void setup_window(const Window& /*window*/) {}

    /// @brief Renders the given Widget.
    /// @param widget   Widget to render.
    virtual void render(const Widget& widget) = 0;

protected: // methods
    /// @brief Value Constructor.
    /// @param shader   The Shader used for rendering.
    /// @param texture  The Texture used for rendering.
    explicit RenderComponent(std::shared_ptr<Shader> shader);

    /// @brief Checks if the RenderComponent's Shader has a uniform variable with the given name.
    /// @param name Name of the uniform to check for.
    bool assert_uniform(const char *name) const;

protected: // fields
    /// @brief The Shader used for rendering.
    std::shared_ptr<Shader> m_shader;
};

} // namespace notf
