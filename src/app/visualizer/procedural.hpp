#pragma once

#include "app/visualizer/visualizer.hpp"

NOTF_OPEN_NAMESPACE

/// Renderer rendering a GLSL fragment shader into a quad.
class ProceduralVisualizer : public Visualizer {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window       Window in which the renderer is displayed.
    /// @param shader_name  Name of a fragment shader to use (file path in relation to the shader directory).
    ProceduralVisualizer(Window& window, const std::string& shader_name);

    /// Destructor.
    ~ProceduralVisualizer() override;

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene providing Properties matching the Shader's uniforms.
    void _visualize(valid_ptr<Scene*> scene) const override;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Renderer to use for visualization.
    FragmentRendererPtr m_renderer;
};

NOTF_CLOSE_NAMESPACE
