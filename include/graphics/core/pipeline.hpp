#pragma once

#include "./gl_forwards.hpp"
#include "common/forwards.hpp"
#include "common/id.hpp"

namespace notf {

//====================================================================================================================//

/// Pipeline ID type.
using PipelineId = IdType<Pipeline, GLuint>;
static_assert(std::is_pod<PipelineId>::value, "PipelineId is not a POD type");

// ===================================================================================================================//

/// Render Pipeline
/// Is not managed by the context, but keeps their Shaders alive.
class Pipeline : public std::enable_shared_from_this<Pipeline> {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Value constructor.
    /// @param context              Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader        Vertex shader to use in the Pipeline.
    /// @param tesselation_shader   Tesselation shader to use in the Pipeline.
    /// @param geometry_shader      Geometry shader to use in the Pipeline.
    /// @param fragment_shader      Fragment shader to use in the Pipeline.
    Pipeline(GraphicsContext& context, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
             GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader);

public:
    /// Factory.
    /// @param context              Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader        Vertex shader to use in the Pipeline.
    /// @param tesselation_shader   Tesselation shader to use in the Pipeline.
    /// @param geometry_shader      Geometry shader to use in the Pipeline.
    /// @param fragment_shader      Fragment shader to use in the Pipeline.
    static PipelinePtr
    create(GraphicsContextPtr& context, VertexShaderPtr vertex_shader, TesselationShaderPtr tesselation_shader,
           GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader);

    static PipelinePtr
    create(GraphicsContextPtr& context, VertexShaderPtr vertex_shader, FragmentShaderPtr fragment_shader)
    {
        return create(context, std::move(vertex_shader), {}, {}, std::move(fragment_shader));
    }

    static PipelinePtr create(GraphicsContextPtr& context, VertexShaderPtr vertex_shader,
                              TesselationShaderPtr tesselation_shader, FragmentShaderPtr fragment_shader)
    {
        return create(context, std::move(vertex_shader), std::move(tesselation_shader), {}, std::move(fragment_shader));
    }

    /// Destructor.
    ~Pipeline();

    /// OpenGL ID of the Pipeline object.
    PipelineId id() const { return m_id; }

    /// Vertex shader attached to this Pipeline.
    const VertexShaderPtr& vertex_shader() const { return m_vertex_shader; }

    /// Tesselation shader attached to this Pipeline.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    const TesselationShaderPtr& tesselation_shader() const { return m_tesselation_shader; }

    /// Geometry shader attached to this Pipeline.
    const GeometryShaderPtr& geometry_shader() const { return m_geometry_shader; }

    /// Fragment shader attached to this Pipeline.
    const FragmentShaderPtr& fragment_shader() const { return m_fragment_shader; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Graphics context containing this Pipeline.
    GraphicsContext& m_graphics_context;

    /// OpenGL ID of the Pipeline object.
    PipelineId m_id;

    /// Vertex shader attached to this Pipeline.
    VertexShaderPtr m_vertex_shader;

    /// Tesselation shader attached to this Pipeline.
    /// The tesselation stage actually contains two shader sources (tesselation control and -evaluation).
    TesselationShaderPtr m_tesselation_shader;

    /// Geometry shader attached to this Pipeline.
    GeometryShaderPtr m_geometry_shader;

    /// Fragment shader attached to this Pipeline.
    FragmentShaderPtr m_fragment_shader;
};

} // namespace notf
