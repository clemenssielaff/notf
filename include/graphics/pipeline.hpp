#pragma once

#include "common/forwards.hpp"
#include "graphics/gl_forwards.hpp"

namespace notf {

/// @brief Render Pipeline
/// Is not managed by the context, but keeps their Shaders alive.
class Pipeline : public std::enable_shared_from_this<Pipeline> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Pipeline stages.
    enum class Stage {
        VERTEX,   ///< Vertex stage
        GEOMETRY, ///< Geometry stage
        FRAGMENT, ///< Fragment stage
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// @brief Value constructor.
    /// @param context          Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader    Vertex shader to use in the Pipeline.
    /// @param geometry_shader  Geometry shader to use in the Pipeline.
    /// @param fragment_shader  Fragment shader to use in the Pipeline.
    Pipeline(GraphicsContext& context, VertexShaderPtr vertex_shader, GeometryShaderPtr geometry_shader,
             FragmentShaderPtr fragment_shader);

public:
    /// @brief Factory
    /// @param context          Graphics context containing this Pipeline and all of its Shader.
    /// @param vertex_shader    Vertex shader to use in the Pipeline.
    /// @param geometry_shader  Geometry shader to use in the Pipeline.
    /// @param fragment_shader  Fragment shader to use in the Pipeline.
    static PipelinePtr create(GraphicsContext& context, VertexShaderPtr vertex_shader,
                              GeometryShaderPtr geometry_shader, FragmentShaderPtr fragment_shader);
    static PipelinePtr
    create(std::unique_ptr<GraphicsContext>& context, VertexShaderPtr vertex_shader, FragmentShaderPtr fragment_shader)
    {
        return create(*context, std::move(vertex_shader), {}, std::move(fragment_shader));
    }

    /// @brief Destructor.
    ~Pipeline();

    /// @brief OpenGL ID of the Pipeline object.
    GLuint id() const { return m_id; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Graphics context containing this Pipeline.
    GraphicsContext& m_graphics_context;

    /// @brief OpenGL ID of the Pipeline object.
    GLuint m_id;

    /// @brief Vertex shader attached to this Pipeline.
    VertexShaderPtr m_vertex_shader;

    /// @brief Geometry shader attached to this Pipeline.
    GeometryShaderPtr m_geometry_shader;

    /// @brief Fragment shader attached to this Pipeline.
    FragmentShaderPtr m_fragment_shader;
};

} // namespace notf
