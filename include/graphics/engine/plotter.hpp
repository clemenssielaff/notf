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
public:
    /// @brief Information necessary to draw a predefined stroke.
    struct StrokeInfo {
        float width;
    };

    /// @brief Information necessary to draw a predefined shape.
    struct ConvexFillInfo {

    };

    /// @brief Information necessary to draw a predefined shape.
    struct ConcaveFillInfo {

    };

private:
    /// @brief Unparsed call to draw a stroke.
    struct StrokeCall {
        StrokeInfo info;

        CubicBezier2f spline;
    };

    /// @brief Unparsed call to draw a convex shape.
    struct ConvexFillCall {
        ConvexFillInfo info;
    };

    /// @brief Unparsed call to draw a concave shape.
    struct ConcaveFillCall {
        ConcaveFillInfo info;
    };

    /// @brief Data structure used by the plotter to store abstract draw calls before parsing them.
    using Call = std::variant<StrokeCall, ConvexFillCall, ConcaveFillCall>;

    /// @brief A batch is a sequence of indices, building one or more patches.
    /// Batches are created when Calls are parsed.
    /// This way, we can group subsequent draw calls of the same type into a batch and render them using a single OpenGL
    /// draw call (for example, to render multiple lines).
    struct Batch {

        using Info = std::variant<StrokeInfo, ConvexFillInfo, ConcaveFillInfo>;

        /// @brief Additional information on how to draw the patches contained in this batch.
        Info info;

        /// @brief Offset of the first index of the batch.
        uint offset;

        /// @brief Number of indices in the batch.
        int size;
    };

    /// @brief Type of the patch to draw.
    enum PatchType : int {
        CONVEX      = 1,
        CONCAVE     = 2,
        SEGMENT     = 3,
        JOINT       = 4,
        START_CAP   = 5,
        END_CAP     = 6,
    };

    /// @brief State of the shader pipeline.
    /// The plotter keeps the state around so it doesn't make any superfluous OpenGL updates.
    /// Is initialized to all invalid values.
    struct State {

        /// @brief Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        /// @brief Stroke width uniform.
        float stroke_width = 0;
    };


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
    /// @param info     Information on how to draw the stroke.
    /// @param spline   Spline to stroke.
    void add_stroke(StrokeInfo info, CubicBezier2f spline)
    {
        if (spline.segments.empty() || info.width <= 0.f) {
            return; // early out
        }
        // TODO: stroke_width less than 1 should set a uniform that fades the line out
        m_calls.emplace_back(StrokeCall{std::move(info), std::move(spline)});
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

    /// @brief Call buffer.
    std::vector<Call> m_calls;

    /// @brief Draw batches.
    std::vector<Batch> m_batches;

    /// @brief State of the plotter pipeline.
    State m_state;
};

} // namespace notf
