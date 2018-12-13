#pragma once

#include <variant>
#include <vector>

#include "notf/meta/id.hpp"
#include "notf/meta/pointer.hpp"
#include "notf/meta/smart_ptr.hpp"

#include "notf/common/aabr.hpp"
#include "notf/common/color.hpp"
#include "notf/common/matrix3.hpp"
#include "notf/common/vector2.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// plotter ========================================================================================================== //

/// Manager for rendering 2D vector graphics.
/// Conceptually, the Plotter knows of two pictures: the one that you can easily (and cheaply) draw on the screen using
/// `render`, and the "buffer" one that is in the process of being defined through the various `add_*` functions.
/// After calling `parse`, the render image is replaced by the new one and the buffer is cleared.
/// Technically, the conceptual images consist of OpenGl buffers and draw calls.
class Plotter {

    // types ----------------------------------------------------------------------------------- //
public:
    /// A Path identifies a range in the Plotters buffer and associates it with additional information.
    /// A shared pointer to a Path object is returned by the Plotter's add_* - methods, so the user can re-use the same
    /// Path more than once.
    class Path {
        friend class Plotter;

        // methods ------------------------------------------------------------
    private:
        NOTF_CREATE_SMART_FACTORIES(Path);

        /// Default constructor.
        Path() = default;

    public:
        /// Destructor.
        ~Path() = default;

        // fields -------------------------------------------------------------
    private:
        /// Offset into the Plotter's index buffer where this Path's indices begin.
        uint offset = 0;

        /// Size of this Path in the Plotter's index buffer.
        int size = 0;

        /// Center of this Path.
        V2f center = V2f::zero();

        /// Whether this Path is convex or concave.
        bool is_convex = true;

        /// Whether this Path is closed or not.
        bool is_closed = true;
    };
    using PathPtr = std::shared_ptr<Path>;

    /// Id identifying a Path in the Design.
    using PathId = IdType<Path, uint>;

    /// Paint is an structure holding information about a particular draw call.
    /// Most of the paint fields are used to initialize the fragment uniforms in the Plotter's shader.
    class Paint {

        // types --------------------------------------------------------------
    public:
        /// Type of cap used at the end of a painted line.
        enum class LineCap : unsigned char {
            BUTT,
            ROUND,
            SQUARE,
        };

        /// Type of joint beween two painted line segments.
        enum class LineJoin : unsigned char {
            MITER,
            ROUND,
            BEVEL,
        };

        /// Winding direction of a painted Shape.
        enum class Winding : unsigned char {
            CCW,
            CW,
            COUNTERCLOCKWISE = CCW,
            CLOCKWISE = CW,
            SOLID = CCW,
            HOLE = CW,
        };

        // methods ------------------------------------------------------------
    public:
        /// Default Constructor.
        Paint() = default;

        /// Value Constructor.
        /// @param color    Single color to paint.
        Paint(Color color) : inner_color(std::move(color)), outer_color(inner_color) {}

        /// Creates a linear gradient.
        static Paint linear_gradient(const V2f& start_pos, const V2f& end_pos, Color start_color, Color end_color);

        static Paint radial_gradient(const V2f& center, float inner_radius, float outer_radius, Color inner_color,
                                     Color outer_color);

        static Paint box_gradient(const V2f& center, const Size2f& extend, float radius, float feather,
                                  Color inner_color, Color outer_color);

        static Paint
        texture_pattern(const V2f& origin, const Size2f& extend, TexturePtr texture, float angle, float alpha);

        /// Turns the Paint into a single solid color.
        void set_color(Color color) {
            xform = M3f::identity();
            radius = 0;
            feather = 1;
            inner_color = std::move(color);
            outer_color = inner_color;
        }

        // fields -------------------------------------------------------------
    public:
        /// Local transform of the Paint.
        M3f xform = M3f::identity();

        /// Texture used within this Paint, can be empty.
        TexturePtr texture;

        /// Inner gradient color.
        Color inner_color = Color::black();

        /// Outer gradient color.
        Color outer_color = Color::black();

        /// Extend of the Paint.
        Size2f extent = Size2f::zero();

        float radius = 0;

        float feather = 1;
    };

    /// Information necessary to draw a predefined stroke.
    struct StrokeInfo {

        /// Transformation applied to the stroked Path.
        M3f transform;

        /// Path to draw.
        std::shared_ptr<Path> path;

        /// Width of the stroke in pixels.
        float width;
    };

    /// Information necessary to fill a Path.
    struct FillInfo {

        /// Path to draw.
        std::shared_ptr<Path> path;

        /// Transformation applied to the filled Path.
        M3f transform;
    };

    struct TextInfo {

        /// Path to draw.
        std::shared_ptr<Path> path;

        /// Font to draw the text in.
        FontPtr font;

        /// Start point of the baseline on which to draw the text.
        V2f translation;
    };

private:
    /// A DrawCall is a sequence of indices, building one or more patches.
    /// This way, we can group subsequent calls of the same type into a batch and render them using a single OpenGL
    /// draw call (for example, to render multiple lines of the same width, color etc.).
    using DrawCall = std::variant<StrokeInfo, FillInfo, TextInfo>;

    /// Type of the patch to draw.
    enum PatchType : int {
        CONVEX = 1,
        CONCAVE = 2,
        STROKE = 3,
        TEXT = 4,
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
        float stroke_width = -1;

        /// Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        V2f vec2_aux1;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Plotter);

    /// Construct a new Plotter.
    /// @param context  GraphicsContext to create the VAO in.
    /// @throws runtime_error   If the OpenGL VAO could not be generated.
    Plotter(GraphicsContext& context);

    /// Destructor.
    ~Plotter();

    /// Registers a new Bezier spline to the Plotter.
    /// @param spline   Spline to register.
    /// @returns        A Path assocaited with the given spline.
    PathPtr add(const CubicBezier2f& spline);

    /// Adds a new polygon shape to draw into the bufffer.
    /// @param polygon  Polygon to register.
    /// @returns        A Path assocaited with the given polygon.
    PathPtr add(const Polygonf& polygon);

    /// Creates a stroke tracing the given Path.
    /// @param path     Path to stroke.
    /// @param info     Information on how to draw the stroke.
    void stroke(valid_ptr<PathPtr> path, StrokeInfo info);

    /// Creates a filled shape bounded by the given Path.
    /// @param path     Path to fill.
    /// @param info     Information on how to draw the shape.
    void fill(valid_ptr<PathPtr> path, FillInfo info);

    /// Adds a new line of text to render into the buffer.
    /// @param text     Text to render.
    /// @param info     Information on how to render the text.
    void write(const std::string& text, TextInfo info);

    /// Replaces the current list of OpenGL draw calls with one parsed from the buffer.
    /// Clears the buffer.
    void swap_buffers();

    /// Clears the buffer without parsing it.
    void clear();

    /// Render the current contents of the Plotter.
    void render() const;

    // TODO: transformation should be a uniform available in the Plotter, so you can draw the same shapes multiple times
    // TODO: can we support automatic instancing, if you draw the same Path multiple times with different transforms?
    // TODO: would it be possible to keep some Paths around between frames? This way we wouldn't have to refill the
    //       buffers every time. This should work similarly to how prefabs work.

private:
    void _render_line(const StrokeInfo& text) const;
    void _render_shape(const FillInfo& text) const;
    void _render_text(const TextInfo& text) const;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// GraphicsContext used to initalize the Plotter.
    /// Since VAOs are not shared, all Plotter operations need to happen within this context.
    GraphicsContext& m_context;

    /// Shader Program used to render the strokes, shapes and glyphs.
    ShaderProgramPtr m_program;

    /// Patch vertices.
    VertexArrayTypePtr m_vertices;

    /// Index of the vertices.
    IndexArrayTypePtr m_indices;

    /// Draw Calls.
    std::vector<DrawCall> m_drawcalls;

    /// Buffer for new Draw Calls.
    std::vector<DrawCall> m_drawcall_buffer;

    /// OpenGL handle of the internal vertex array object.
    GLuint m_vao_id = 0;

    /// State of the Plotter pipeline.
    mutable State m_state;
};

NOTF_CLOSE_NAMESPACE
