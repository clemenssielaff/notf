#pragma once

#include "notf/common/color.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/common/geo/size2.hpp"

#include "notf/graphic/vertex_object.hpp"

NOTF_OPEN_NAMESPACE

// plotter ========================================================================================================== //

class Plotter {

    // TODO: refactor paint and fragmentpaint

    // types ----------------------------------------------------------------------------------- //
public:
    /// Paint is an structure holding information about a particular draw call.
    /// Most of the paint fields are used to initialize the fragment uniforms in the Plotter's shader.
    class Paint {

        // methods ------------------------------------------------------------
    public:
        /// Default Constructor.
        Paint() = default;

        /// Value Constructor.
        /// @param color    Single solid color to paint.
        Paint(Color color) : inner_color(std::move(color)), outer_color(inner_color) {}

        /// Creates a linear gradient.
        static Paint linear_gradient(const V2f& start_pos, const V2f& end_pos, Color start_color, Color end_color);

        static Paint radial_gradient(const V2f& center, float inner_radius, float outer_radius, Color inner_color,
                                     Color outer_color);

        static Paint box_gradient(const V2f& center, const Size2f& extend, float radius, float feather,
                                  Color inner_color, Color outer_color);

        static Paint texture_pattern(const V2f& origin, const Size2f& extend, TexturePtr texture, float angle,
                                     float alpha);

        /// Turns the Paint into a single solid color.
        void set_color(Color color) {
            xform = M3f::identity();
            gradient_radius = 0;
            feather = 1;
            inner_color = std::move(color);
            outer_color = inner_color;
        }

        /// Equality comparison operator.
        /// @param other    Paint to compare against.
        bool operator==(const Paint& other) const noexcept {
            return texture == other.texture                             //
                   && is_approx(gradient_radius, other.gradient_radius) //
                   && is_approx(feather, other.feather)                 //
                   && extent.is_approx(other.extent)                    //
                   && xform.is_approx(other.xform)                      //
                   && outer_color.is_approx(other.outer_color)          //
                   && inner_color.is_approx(other.inner_color);
        }

        /// Inequality operator
        /// @param other    Paint to compare against.
        constexpr bool operator!=(const Paint& other) const noexcept { return !operator==(other); }

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

        float gradient_radius = 0;

        float feather = 1;
    };

    /// Clipping, is a rotated rectangle that limits the plotted area.
    /// Clipping information is stored in a UniformBuffer.
    struct Clipping {
        M3f xform = M3f::identity();
        Size2f size = Size2f::invalid(); // TODO test clipping
    };

    /// Type of cap used at the end of a painted line.
    enum class LineCap : unsigned char {
        _CURRENT, // default value, means "use the current one"
        BUTT,
        ROUND,
        SQUARE,
    };

    /// Type of joint beween two painted line segments.
    enum class LineJoin : unsigned char {
        _CURRENT, // default value, means "use the current one"
        MITER,
        ROUND,
        BEVEL,
    };

    /// Winding direction of a painted Shape.
    enum class Winding : unsigned char {
        _CURRENT, // default value, means "use the current one"
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE = CW,
        SOLID = CCW,
        HOLE = CW,
    };

private:
    /// A Path identifies a range in the Plotters buffer and associates it with additional information.
    struct Path {

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

    // draw calls -------------------------------------------------------------

    struct _DrawCallBase {

        /// Index of the Path to draw.
        uint path_index;

        /// Index of the Paint in the UniformBuffer.
        uint paint_index;

        /// Index of the Clip in the UniformBuffer.
        uint clip_index;

        /// Index of the 2D transformation to apply.
        uint xform_index;
    };

    struct FillCall : public _DrawCallBase {
        Winding winding = Winding::_CURRENT;
    };

    struct StrokeCall : public _DrawCallBase {

        /// Width of the stroke in pixels.
        float width;

        LineCap cap = LineCap::_CURRENT;

        LineJoin join = LineJoin::_CURRENT;
    };

    struct WriteCall : public _DrawCallBase {

        /// Font to draw the text in.
        FontPtr font;
    };

    using DrawCall = std::variant<StrokeCall, FillCall, WriteCall>;

    // internal buffers -------------------------------------------------------

    struct _VertexPosAttribute {
        using type = V2f;
    };

    struct _LeftCtrlAttribute {
        using type = V2f;
    };

    struct _RightCtrlAttribute {
        using type = V2f;
    };

    struct _InstanceXformAttribute {
        using type = M3f;
    };

public:
    using VertexBuffer = vertex_buffer_t<_VertexPosAttribute, _LeftCtrlAttribute, _RightCtrlAttribute>;
    using IndexBuffer = IndexBuffer<GLuint>;
    using InstanceBuffer = vertex_buffer_t<_InstanceXformAttribute>;

private:
    using VertexBufferPtr = std::shared_ptr<VertexBuffer>;
    using IndexBufferPtr = std::shared_ptr<IndexBuffer>;
    using InstanceBufferPtr = std::shared_ptr<InstanceBuffer>;

    // fragment paint ---------------------------------------------------------

    /// For details on uniform buffer struct size and alignment issues see Sub-section 2.15.3.1.2 in:
    /// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_uniform_buffer_object.txt
    struct FragmentPaint {

        // types ----------------------------------------------------------------------------------- //
    public:
        enum class Type : GLint {
            GRADIENT = 0,
            IMAGE = 1,
            STENCIL = 2,
            TEXT = 3,
        };

        // methods --------------------------------------------------------------------------------- //
    public:
        /// @{
        /// Constructor
        FragmentPaint(const Paint& paint, const Clipping& clipping, const float stroke_width, const Type type);
        FragmentPaint(const Paint& paint, const Type type = Type::GRADIENT) : FragmentPaint(paint, {}, 0, type) {}
        /// @}

        // fields ---------------------------------------------------------------------------------- //
    public:                                          // offset in basic machine units
        std::array<float, 4> paint_rotation = {};    //  0 (size = 4)
        std::array<float, 2> paint_translation = {}; //  4 (size = 2)
        std::array<float, 2> paint_size = {};        //  6 (size = 2)
        std::array<float, 4> clip_rotation = {};     //  8 (size = 4)
        std::array<float, 2> clip_translation = {};  // 12 (size = 2)
        std::array<float, 2> clip_size = {1, 1};     // 14 (size = 2)
        Color inner_color = Color::transparent();    // 16 (size = 4)
        Color outer_color = Color::transparent();    // 20 (size = 4) TODO Color could be a float[3]
        Type type;                                   // 24 (size = 1)
        float stroke_width;                          // 25 (size = 1)
        float gradient_radius = 0;                   // 26 (size = 1)
        float feather = 0;                           // 27 (size = 1)
    };                                               // total size is: 28
    friend ::std::hash<FragmentPaint>;

    // convenience enums ------------------------------------------------------

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

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Plotter);

    /// Construct a new Plotter.
    /// @param context      GraphicsContext to create the VAO in.
    /// @throws OpenGLError If any of the OpenGL buffers could not be generated.
    Plotter(GraphicsContext& context);

    /// Destructor.
    ~Plotter();

    /// Registers a new Bezier spline to the Plotter.
    /// @param spline   Spline to draw.
    void set_shape(const CubicBezier2f& spline);

    /// Adds a new polygon shape to draw into the bufffer.
    /// @param polygon  Polygon to draw.
    void set_shape(const Polygonf& polygon);

    /// Sets the new paint of the Plotter.
    /// @paint  New paint.
    void set_paint(const Paint& paint);

    /// Sets the 2D transformation of the Plotter.
    /// @param xform    New transformation.
    void set_xform(M3f xform);

    /// Fill the current Path.
    void fill();

    /// Creates a stroke tracing the current Path.
    void stroke();

    /// Writes a new line of text.
    /// @param text     Text to render.
    void write(std::string_view text);

    /// Resets the Plotter without rendering.
    void reset();
    // TODO: better: start_frame, cancel_frame, end_frame ?

    /// Render all stored draw calls.
    void render() const;

private:
    /// Fill implementation.
    void _fill(const FillCall& call);

    /// Stroke implementation.
    void _stroke(const StrokeCall& call);

    /// Write implementation.
    void _write(const WriteCall& call);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// GraphicsContext used to initalize the Plotter.
    /// Since VAOs are not shared, all Plotter operations need to happen within this context.
    GraphicsContext& m_context;

    /// Shader Program used to render the strokes, shapes and glyphs.
    ShaderProgramPtr m_program;

    /// Buffer storing the vertices used to construct paths and glyphs.
    VertexBufferPtr m_vertex_buffer;

    /// Indices into the vertex buffer.
    IndexBufferPtr m_index_buffer;

    /// Buffer containing instance transformations.
    InstanceBufferPtr m_instance_buffer;

    /// Internal vertex object managing attribute- and buffer bindings.
    VertexObjectPtr m_vertex_object;

    /// Uniform buffer containing the paint uniform blocks.
    UniformBufferPtr<FragmentPaint> m_paint_buffer;

    /// Uniform buffer containing the clipping uniform blocks.
    UniformBufferPtr<Clipping> m_clipping_buffer;

    /// All paths.
    std::vector<Path> m_paths;

    /// Draw Calls.
    std::vector<DrawCall> m_drawcalls;

    /// State of the shader pipeline.
    /// The plotter keeps the state around so it doesn't make any superfluous OpenGL updates.
    /// Is initialized to all invalid values.
    struct State {
        /// Screen size.
        Size2i screen_size = Size2i::zero();

        /// Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        LineCap line_cap = LineCap::_CURRENT; // TODO: make use of line cap

        LineJoin line_join = LineJoin::_CURRENT; // TODO: make use of line join

        Winding winding = Winding::_CURRENT; // TODO: make use of Path winding

        /// How many indices to feed into a patch.
        int patch_vertices = 2;

        /// Stroke width uniform.
        float stroke_width = -1;

        /// Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        V2f vec2_aux1;

        /// Index at which the paint buffer is bound.
        uint paint_index = 0;

        /// Index at which the clipping buffer is bound.
        uint clip_index = 0;

    } m_state;
};

NOTF_CLOSE_NAMESPACE
