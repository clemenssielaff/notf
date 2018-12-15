#pragma once

#include "notf/app/graph/visualizer.hpp"

NOTF_OPEN_NAMESPACE

// widget visualizer ================================================================================================ //

/// Visualizer used to visualize a WidgetScene.
class WidgetVisualizer : public Visualizer {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param context  GraphicsContext to operate in.
    WidgetVisualizer(GraphicsContext& context);

    /// Destructor.
    ~WidgetVisualizer() override;

    /// Subclass-defined visualization implementation.
    /// @param scene    Scene providing Properties matching the Shader's uniforms.
    void visualize(valid_ptr<Scene*> scene) const override;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Painterpreter to use for visualization.
    PainterpreterPtr m_painterpreter;
};

NOTF_CLOSE_NAMESPACE
