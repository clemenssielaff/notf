#pragma once

#include "graphics/ids.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Render Pipeline
/// Is not managed by the context, but keeps their Shaders alive.
class Pipeline : public std::enable_shared_from_this<Pipeline> {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Value constructor.
    /// @param context              Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader        Vertex shader to use in the Pipeline.
    /// @param tesselation_shader   Tesselation shader to use in the Pipeline.
    /// @param geometry_shader      Geometry shader to use in the Pipeline.
    /// @param fragment_shader      Fragment shader to use in the Pipeline.
    Pipeline(GraphicsContext& context, VertexShaderPtr get_vertex_shader, TesselationShaderPtr get_tesselation_shader,
             GeometryShaderPtr get_geometry_shader, FragmentShaderPtr get_fragment_shader);

public:
    /// Factory.
    /// @param context              Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader        Vertex shader to use in the Pipeline.
    /// @param tesselation_shader   Tesselation shader to use in the Pipeline.
    /// @param geometry_shader      Geometry shader to use in the Pipeline.
    /// @param fragment_shader      Fragment shader to use in the Pipeline.
    static PipelinePtr
    create(GraphicsContext& context, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
           GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(Pipeline, context, std::move(vertex_shader), std::move(tesselation_shader),
                                             std::move(geometry_shader), std::move(fragment_shader));
    }

    static PipelinePtr
    create(GraphicsContext& context, VertexShaderPtr vertex_shader, FragmentShaderPtr fragment_shader)
    {
        return create(context, std::move(vertex_shader), {}, {}, std::move(fragment_shader));
    }

    static PipelinePtr create(GraphicsContext& context, VertexShaderPtr vertex_shader,
                              TesselationShaderPtr tesselation_shader, FragmentShaderPtr fragment_shader)
    {
        return create(context, std::move(vertex_shader), std::move(tesselation_shader), {}, std::move(fragment_shader));
    }

    /// Destructor.
    ~Pipeline();

    /// OpenGL ID of the Pipeline object.
    PipelineId get_id() const { return m_id; }

    /// Vertex shader attached to this Pipeline.
    const VertexShaderPtr& get_vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this Pipeline.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    const TesselationShaderPtr& get_tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this Pipeline.
    const GeometryShaderPtr& get_geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this Pipeline.
    const FragmentShaderPtr& get_fragment_shader() const { return m_fragment_shader; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Graphics context containing this Pipeline.
    GraphicsContext& m_graphics_context;

    /// Vertex shader attached to this Pipeline.
    VertexShaderPtr m_vertex_shader;

    /// Tesselation shader attached to this Pipeline.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    TesselationShaderPtr m_tesselation_shader;

    /// Geometry shader attached to this Pipeline.
    GeometryShaderPtr m_geometry_shader;

    /// Fragment shader attached to this Pipeline.
    FragmentShaderPtr m_fragment_shader;

    /// OpenGL ID of the Pipeline object.
    PipelineId m_id = 0;
};

NOTF_CLOSE_NAMESPACE
