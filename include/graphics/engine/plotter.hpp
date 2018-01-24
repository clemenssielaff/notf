#pragma once

#include "common/forwards.hpp"
#include "common/size2.hpp"
#include "common/variant.hpp"
#include "common/vector2.hpp"
#include "common/warnings.hpp"
#include "graphics/core/gl_forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// @brief Manager for rendering 2D vector graphics.
/// Conceptually, the Plotter knows of two pictures: the one that you can easily (and cheaply) draw on the screen using
/// `render`, and the "buffer" one that is in the process of being defined through the various `add_*` functions.
/// After calling `parse`, the render image is replaced by the new one and the buffer is cleared.
/// Technically, the conceptual images consist of OpenGl buffers and draw calls.
class Plotter {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Information necessary to draw a predefined stroke.
    struct StrokeInfo {
        float width;
    };

    /// @brief Information necessary to draw a predefined shape.
    struct ShapeInfo {
        friend class Plotter;

    private:
        Vector2f center;

        bool is_convex;

        PADDING(3)
    };

    struct TextInfo {
        /// @brief Font to draw the text in.
        FontPtr font;

        /// @brief Start point of the baseline on which to draw the text.
        Vector2f translation;
    };

private:
    /// @brief A batch is a sequence of indices, building one or more patches.
    /// This way, we can group subsequent draw calls of the same type into a batch and render them using a single OpenGL
    /// draw call (for example, to render multiple lines of the same width, color etc.).
    struct Batch {

        using Info = std::variant<StrokeInfo, ShapeInfo, TextInfo>;

        /// @brief Additional information on how to draw the patches contained in this batch.
        Info info;

        /// @brief Offset of the first index of the batch.
        uint offset;

        /// @brief Number of indices in the batch.
        int size;
    };

    /// @brief Type of the patch to draw.
    enum PatchType : int {
        CONVEX  = 1,
        CONCAVE = 2,
        STROKE  = 3,
        TEXT    = 4,
        // JOINT     = 31, // internal
        // START_CAP = 32, // internal
        // END_CAP   = 33, // internal
    };

    /// @brief State of the shader pipeline.
    /// The plotter keeps the state around so it doesn't make any superfluous OpenGL updates.
    /// Is initialized to all invalid values.
    struct State {
        /// @brief Screen size.
        Size2i screen_size = Size2i::zero();

        /// @brief Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        /// @brief How many indices to feed into a patch.
        int patch_vertices = 2;

        /// @brief Stroke width uniform.
        float stroke_width = 0;

        /// @brief Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        Vector2f vec2_aux1;
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

    /// @brief Replaces the current list of OpenGL draw calls with one parsed from the buffer.
    /// Clears the buffer.
    void apply();

    /// @brief Clears the buffer without parsing it.
    void clear();

    /// @brief Render the current contents of the Plotter.
    void render();

    /// @brief Adds a new Bezier spline to stroke into the bufffer.
    /// @param info     Information on how to draw the stroke.
    /// @param spline   Spline to stroke.
    void add_stroke(StrokeInfo info, const CubicBezier2f& spline);

    /// @brief Adds a new shape to draw into the bufffer.
    /// @param info     Information on how to draw the shape.
    /// @param spline   Shape to draw.
    void add_shape(ShapeInfo info, const Polygonf& polygon);

    /// @brief Adds a new line of text to render into the buffer.
    /// @param info     Information on how to render the text.
    /// @param text     Text to render.
    void add_text(TextInfo info, const std::string& text);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Graphics Context in which the Plotter lives.
    GraphicsContext& m_graphics_context;

    /// @brief Shader pipeline used to render the strokes, shapes and glyphs.
    PipelinePtr m_pipeline;

    /// @brief OpenGL handle of the internal vertex array object.
    GLuint m_vao_id;

    /// @brief Patch vertices.
    VertexArrayTypePtr m_vertices;

    /// @brief Index of the vertices.
    IndexArrayTypePtr m_indices;

    /// @brief Draw batches.
    std::vector<Batch> m_batches;

    /// @brief Buffer for new batches.
    std::vector<Batch> m_batch_buffer;

    /// @brief State of the Plotter pipeline.
    State m_state;
};

} // namespace notf