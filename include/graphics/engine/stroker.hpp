#pragma once

#include "common/bezier.hpp"
#include "common/forwards.hpp"
#include "graphics/core/gl_forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// @brief Manager for rendering 2D lines.
/// Manages the shader and -invokation, as well as the rendered vertices.
class Stroker final {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(Stroker)

    /// @brief Construct a new Stroker.
    /// @param context  Graphics context.
    /// @throws std::runtime_error   If the OpenGL VAO could not be generated.
    Stroker(GraphicsContextPtr& context);

    /// @brief Destructor.
    ~Stroker();

    /// @brief Adds a new Bezier spline to stroke.
    /// @param spline   Spline to add.
    void add_spline(CubicBezier2f spline);

    /// @brief Clears the current contents of the Stroker and applies the new ones.
    /// New strokes are all that have been added via `add_stroke` since the last call to `apply`.
    void apply_new();

    /// @brief Discards all new strokes added with `apply_new` without applying them.
    void discard_new() { m_spline_buffer.clear(); }

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

    /// @brief Buffer into which bezier segments are stored while the UI is drawn (not rendered).
    std::vector<CubicBezier2f> m_spline_buffer;
};

} // namespace notf
