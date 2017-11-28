#pragma once

#include "common/forwards.hpp"
#include "graphics/gl_forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// @brief Manager for rendering 2D lines.
/// Manages the shader and -invokation, as well as the rendered vertices.
class Stroker final {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(Stroker)

    /// @brief Construct a new Stroker.
    /// @param context  Graphics context.
    /// @throws std::runtime_error   If the OpenGL VAO could not be generated.
    Stroker(GraphicsContextPtr& context);

    /// @brief Destructor.
    ~Stroker();

    /// @brief Render the current contents of the Stroker.
    void render();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Graphics Context in which the Texture lives.
    GraphicsContext& m_graphics_context;

    /// @brief Shader pipeline used to render the lines.
    PipelinePtr m_pipeline;

    /// @brief OpenGL handle of the internal vertex array object.
    GLuint m_vao_id;

    /// @brief Rendered vertices.
    VertexArrayTypePtr m_vertices;

    /// @brief Index of the vertices.
    IndexArrayTypePtr m_indices;
};

} // namespace notf
