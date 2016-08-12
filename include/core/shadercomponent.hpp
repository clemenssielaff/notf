#pragma once

#include "core/component.hpp"
#include "core/glfw_wrapper.hpp"

namespace signal {

class ShaderComponent : public Component {

protected:
    explicit ShaderComponent();

public:
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::TEXTURE; }

    /// \brief Updates this Component.
    virtual void update() override;

private: // fields
    // TEMP
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    float test_offset;
};

} // namespace signal
