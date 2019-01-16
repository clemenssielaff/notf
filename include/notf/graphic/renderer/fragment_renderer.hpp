#pragma once

#include "notf/meta/pointer.hpp"

#include "notf/graphic/shader_program.hpp"

NOTF_OPEN_NAMESPACE

// fragment renderer ================================================================================================ //

/// Renderer rendering a GLSL fragment shader into a quad.
class FragmentRenderer {

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(FragmentRenderer);

    /// Constructor.
    /// @param context      Graphics context in which to operate.
    /// @param shader_name  Name of a fragment shader to use (file path in relation to the shader directory).
    FragmentRenderer(valid_ptr<VertexShaderPtr> vertex_shader, valid_ptr<FragmentShaderPtr> fragment_shader);

    template<class T>
    void set_uniform(const std::string& name, T&& value) {
        m_program->set_uniform(name, std::forward<T>(value));
    }

    /// Renders the fragment shader into a fullscreen quad.
    void render() const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Shader Program used to produce the graphics.
    ShaderProgramPtr m_program;

    /// Fragment shader.
    const FragmentShaderPtr& m_fragment_shader;
};

NOTF_CLOSE_NAMESPACE
