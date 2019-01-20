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
    /// @param context          Graphics context in which to operate.
    /// @param vertex_shader    VertexShader to use.
    /// @param fragment_shader  FragmentShader to use.
    FragmentRenderer(GraphicsContext& context, valid_ptr<VertexShaderPtr> vertex_shader,
                     valid_ptr<FragmentShaderPtr> fragment_shader);

    template<class T>
    void set_uniform(const std::string& name, T&& value) {
        m_program->get_uniform(name).set(std::forward<T>(value));
    }

    /// Renders the fragment shader into a fullscreen quad.
    void render() const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// ShaderProgram used to produce the graphics.
    ShaderProgramPtr m_program;
};

NOTF_CLOSE_NAMESPACE
