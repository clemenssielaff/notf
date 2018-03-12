#pragma once

#include "app/graphics_producer.hpp"

NOTF_OPEN_NAMESPACE

class FragmentProducer : public GraphicsProducer {
    friend class GraphicsProducer;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token    Token to make sure that the instance can only be created by a call to `_create`.
    /// @param manager  SceneManager.
    /// @param shader   Name of a fragment shader to use.
    FragmentProducer(const Token& token, SceneManagerPtr& manager, const std::string& shader);

public:
    NOTF_NO_COPY_OR_ASSIGN(FragmentProducer)

    /// Factory.
    /// @param manager  SceneManager.
    /// @param shader   Name of a fragment shader to use.
    static FragmentProducerPtr create(SceneManagerPtr& manager, const std::string& shader)
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

NOTF_CLOSE_NAMESPACE
