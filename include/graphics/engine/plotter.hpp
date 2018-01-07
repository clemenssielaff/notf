#pragma once

#include "common/bezier.hpp"
#include "common/forwards.hpp"
#include "common/variant.hpp"
#include "graphics/core/gl_forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// @brief Manager for rendering 2D vector graphics.
/// The Plotter contains a buffer of `Calls`, command pattern-like objects that represent individual lines or shapes to
/// draw. Widgets fill the buffer in their `paint` methods first. Afterwards, the buffer is parsed by the Plotter and
/// finally rendered onto the screen. After parsing, the buffer is empty but you can easily (and cheaply) re-render the
/// last call list using `render`. To update the rendered image, fill the buffer again and call `parse`.
class Plotter final {

    // types ---------------------------------------------------------------------------------------------------------//
private:
    struct Stroke {
        CubicBezier2f spline;
    };

    struct ConvexFill {};

    struct ConcaveFill {};

    /// @brief Data structure used by the plotter to store abstract draw calls before parsing them.
    using Call = std::variant<Stroke, ConvexFill, ConcaveFill>;

    /// @brief A batch is a sequence of indices, building one or more patches.
    /// Batches are created when Calls are parsed.
    /// This way, we can group subsequent draw calls of the same type into a batch and render them using a single OpenGL
    /// draw call (for example, to render multiple lines).
    struct Batch {
        /// @brief Type of the patches contained in this batch.
        int type;

        /// @brief Offset of the first index of the batch.
        int offset;

        /// @brief Number of indices in the batch.
        int size;
    };

    // TODO: continue here by copying `apply_new` into the "stroke case" of `parse`

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(Plotter)

    /// @brief Construct a new Plotter.
    /// @param context  Graphics context.
    /// @throws std::runtime_error   If the OpenGL VAO could not be generated.
    Plotter(GraphicsContextPtr& context);

    /// @brief Destructor.
    ~Plotter();

    /// @brief Adds a new Bezier spline to stroke.
    /// @param spline   Spline to add.
    void add_spline(CubicBezier2f spline)
    {
        if (!spline.segments.empty()) {
            m_calls.emplace_back(Stroke{std::move(spline)});
        }
    }

    /// @brief Replaces the current list of OpenGL draw calls with one parsed from the buffer.
    /// Clears the buffer.
    void parse();

    /// @brief Clears the call buffer without parsing it.
    void clear() { m_calls.clear(); }

    /// @brief Render the current contents of the Plotter.
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

    /// @brief Draw batches.
    std::vector<Batch> m_batches;

    /// @brief Call buffer.
    std::vector<Call> m_calls;
};

} // namespace notf
