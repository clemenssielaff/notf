#pragma once

#include "app/renderer.hpp"

NOTF_OPEN_NAMESPACE

/// Renderer rendering a GLSL fragment shader into a quad.
class ProceduralRenderer : public Renderer {

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    /// @param context      Graphics context.
    /// @param shader_name  Name of a fragment shader to use (file path in relation to the shader directory).
    ProceduralRenderer(GraphicsContext& context, const std::string& shader_name);

public:
    NOTF_NO_COPY_OR_ASSIGN(ProceduralRenderer);

    /// Factory.
    /// @param window       Window in which the renderer is displayed.
    /// @param shader_name  Name of a fragment shader to use (file path in relation to the shader directory).
    static ProceduralRendererPtr create(Window& window, const std::string& shader_name);

private:
    /// Subclass-defined implementation of the Renderer's rendering.
    virtual void _render() const override;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// GraphicsContext.
    GraphicsContext& m_context;

    /// Shader pipeline used to produce the graphics.
    PipelinePtr m_pipeline;
};

NOTF_CLOSE_NAMESPACE
