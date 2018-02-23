#pragma once

#include "graphics/engine/graphics_producer.hpp"

namespace notf {

class FragmentProducer : public GraphicsProducer {
    friend class GraphicsProducer;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token    Token to make sure that the instance can only be created by a call to `_create`.
    /// @param manager  RenderManager.
    /// @param shader   Name of a fragment shader to use.
    FragmentProducer(const Token& token, RenderManagerPtr& manager, const std::string& shader);

public:
    DISALLOW_COPY_AND_ASSIGN(FragmentProducer)

    /// Factory.
    /// @param manager  RenderManager.
    /// @param shader   Name of a fragment shader to use.
    static FragmentProducerPtr create(RenderManagerPtr& manager, const std::string& shader)
    {
        return _create<FragmentProducer>(manager, shader);
    }

private:
    /// Subclass-defined implementation of the GraphicsProducer's rendering.
    virtual void _render() const override;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Shader pipeline used to produce the graphics.
    PipelinePtr m_pipeline;

    /// GraphicsContext.
    GraphicsContext& m_context;
};

} // namespace notf
