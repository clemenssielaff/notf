#pragma once

#include "app/visualizer/visualizer.hpp"

NOTF_OPEN_NAMESPACE

/// Visualizer used to visualize a WidgetScene.
class WidgetVisualizer : public Visualizer {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    //    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    /// @param window       Window in which the renderer is displayed.
    WidgetVisualizer(Window& window);

    /// Destructor.
    ~WidgetVisualizer() override;

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene providing Properties matching the Shader's uniforms.
    void visualize(valid_ptr<Scene*> scene) const override;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Renderer to use for visualization.
    PlotterPtr m_renderer;
};

NOTF_CLOSE_NAMESPACE
