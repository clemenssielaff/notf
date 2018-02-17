#pragma once

#include "common/forwards.hpp"
#include "common/size2.hpp"
#include "common/variant.hpp"
#include "common/vector2.hpp"
#include "common/warnings.hpp"
#include "graphics/core/gl_forwards.hpp"
#include "graphics/engine/graphics_producer.hpp"

namespace notf {

// ===================================================================================================================//

/// Manager for rendering 2D vector graphics.
/// Conceptually, the Plotter knows of two pictures: the one that you can easily (and cheaply) draw on the screen using
/// `render`, and the "buffer" one that is in the process of being defined through the various `add_*` functions.
/// After calling `parse`, the render image is replaced by the new one and the buffer is cleared.
/// Technically, the conceptual images consist of OpenGl buffers and draw calls.
class Plotter final : public GraphicsProducer {
    friend class GraphicsProducer;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Information necessary to draw a predefined stroke.
    struct StrokeInfo {
        float width;
    };

    /// Information necessary to draw a predefined shape.
    struct ShapeInfo {
        friend class Plotter;

    private:
        Vector2f center;

        bool is_convex;

        PADDING(3)
    };

    struct TextInfo {
        /// Font to draw the text in.
        FontPtr font;

        /// Start point of the baseline on which to draw the text.
        Vector2f translation;
    };

private:
    /// A batch is a sequence of indices, building one or more patches.
    /// This way, we can group subsequent draw calls of the same type into a batch and render them using a single OpenGL
    /// draw call (for example, to render multiple lines of the same width, color etc.).
    struct Batch {

        using Info = std::variant<StrokeInfo, ShapeInfo, TextInfo>;

        /// Additional information on how to draw the patches contained in this batch.
        Info info;

        /// Offset of the first index of the batch.
        uint offset;

        /// Number of indices in the batch.
        int size;
    };

    /// Type of the patch to draw.
    enum PatchType : int {
        CONVEX  = 1,
        CONCAVE = 2,
        STROKE  = 3,
        TEXT    = 4,
        // JOINT     = 31, // internal
        // START_CAP = 32, // internal
        // END_CAP   = 33, // internal
    };

    /// State of the shader pipeline.
    /// The plotter keeps the state around so it doesn't make any superfluous OpenGL updates.
    /// Is initialized to all invalid values.
    struct State {
        /// Screen size.
        Size2i screen_size = Size2i::zero();

        /// Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        /// How many indices to feed into a patch.
        int patch_vertices = 2;

        /// Stroke width uniform.
        float stroke_width = 0;

        /// Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        Vector2f vec2_aux1;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Construct a new Plotter.
    /// @param token            Token to make sure that the instance can only be created by a call to `_create`.
    /// @param render_manager   RenderManager.
    /// @throws runtime_error   If the OpenGL VAO could not be generated.
    Plotter(const Token& token, RenderManager& render_manager);

public:
    DISALLOW_COPY_AND_ASSIGN(Plotter)

    /// Factory.
    /// @param render_manager   RenderManager.
    static PlotterPtr create(RenderManager& render_manager) { return _create<Plotter>(render_manager); }

    /// Destructor.
    virtual ~Plotter() override;

    /// Unique type of this GraphicsProducer subclass.
    virtual Type render_type() const override { return Type::PLOTTER; }

    /// Replaces the current list of OpenGL draw calls with one parsed from the buffer.
    /// Clears the buffer.
    void apply();

    /// Clears the buffer without parsing it.
    void clear();

    /// Render the current contents of the Plotter.
    virtual void render() const override;

    /// Whether the GraphicsProducer is currently dirty or not.
    virtual bool is_dirty() const override { return false; } // TODO: Plotter:is_dirty

    /// Adds a new Bezier spline to stroke into the bufffer.
    /// @param info     Information on how to draw the stroke.
    /// @param spline   Spline to stroke.
    void add_stroke(StrokeInfo info, const CubicBezier2f& spline);

    /// Adds a new shape to draw into the bufffer.
    /// @param info     Information on how to draw the shape.
    /// @param spline   Shape to draw.
    void add_shape(ShapeInfo info, const Polygonf& polygon);

    /// Adds a new line of text to render into the buffer.
    /// @param info     Information on how to render the text.
    /// @param text     Text to render.
    void add_text(TextInfo info, const std::string& text);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Graphics Context in which the Plotter lives.
    GraphicsContext& m_graphics_context;

    /// Font Manager used to render text.
    FontManager& m_font_manager;

    /// Shader pipeline used to render the strokes, shapes and glyphs.
    PipelinePtr m_pipeline;

    /// OpenGL handle of the internal vertex array object.
    GLuint m_vao_id;

    /// State of the Plotter pipeline.
    mutable State m_state;

    /// Patch vertices.
    VertexArrayTypePtr m_vertices;

    /// Index of the vertices.
    IndexArrayTypePtr m_indices;

    /// Draw batches.
    std::vector<Batch> m_batches;

    /// Buffer for new batches.
    std::vector<Batch> m_batch_buffer;
};

} // namespace notf
