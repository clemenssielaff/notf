#pragma once

#include "notf/common/color.hpp"
#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/common/geo/path2.hpp"
#include "notf/common/geo/polyline.hpp"
#include "notf/common/geo/size2.hpp"
#include "notf/common/geo/triangle.hpp"

#include "notf/graphic/vertex_object.hpp"

NOTF_OPEN_NAMESPACE

// plotter ========================================================================================================== //

class Plotter {

    // TODO: refactor paint and fragmentpaint

    // types ----------------------------------------------------------------------------------- //
private:
    // buffer attributes ------------------------------------------------------

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
    // internal buffers -------------------------------------------------------

    /// OpenGL buffer holding path vertices.
    using VertexBuffer = vertex_buffer_t<_VertexPosAttribute, _LeftCtrlAttribute, _RightCtrlAttribute>;
    using VertexBufferPtr = std::shared_ptr<VertexBuffer>;

    /// OpenGL buffer holding path indices.
    using IndexBuffer = IndexBuffer<GLuint>;
    using IndexBufferPtr = std::shared_ptr<IndexBuffer>;

    /// OpenGL buffer holding per-instance 2D transformations.
    using InstanceBuffer = vertex_buffer_t<_InstanceXformAttribute>;
    using InstanceBufferPtr = std::shared_ptr<InstanceBuffer>;

    // paint ------------------------------------------------------------------

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

    // path -------------------------------------------------------------------

    /// 2D Path as held by a PlotterDesign.
    struct Path {
        struct Vertex {
            void set_pos(V2f pos) { std::get<0>(m_vertex) = std::move(pos); }

            void set_left_ctrl(V2f pos) { std::get<1>(m_vertex) = std::move(pos); }

            void set_right_ctrl(V2f pos) { std::get<2>(m_vertex) = std::move(pos); }

            /// Sets the first control point from a spline.
            /// We might have to modify the given ctrl point so we can be certain that is stores the outgoing tangent
            /// of the vertex. For that, we get the tangent either from the ctrl point itself (if the distance to the
            /// vertex > 0) or by calculating the tangent from the spline at the vertex. The control point is then moved
            /// onto the tangent, one unit further away from the vertex than originally. If we encounter a ctrl point in
            /// the shader that is of unit length, we know that in reality it was positioned on the vertex itself and
            /// still get the vertex tangent.
            /// @param spline   Spline on the left of the vertex position.
//            void set_left_ctrl(const CubicBezier2f::Segment& spline);

            /// Sets the second control point from a spline.
            /// See `set_ctrl1` on why this method is needed.
            /// @param spline   Spline on the right of the vertex position.
//            void set_right_ctrl(const CubicBezier2f::Segment& spline);

            // fields ---------------------------------------------------------
        private:
            VertexBuffer::vertex_t m_vertex;
        };


        // fields -------------------------------------------------------------
    private:
        std::vector<Vertex> m_vertices;
        size_t m_hash = 0;
        M3f m_xform = M3f::identity();
    };

    // enums ------------------------------------------------------------------

    /// Type of cap used at the end of a painted line.
    enum class LineCap : uchar {
        _CURRENT, // default value, means "use the current one"
        BUTT,
        ROUND,
        SQUARE,
    };

    /// Type of joint beween two painted line segments.
    enum class LineJoin : uchar {
        _CURRENT, // default value, means "use the current one"
        MITER,
        ROUND,
        BEVEL,
    };

    // external draw calls ----------------------------------------------------

    /// State used to contextualize paint operations.
    struct PainterState {

        /// Transformation of the Painter.
        M3f xform = M3f::identity();

        /// Paint used for painting.
        Paint paint = Color(255, 255, 255);

        /// Current Shape;
        Path2 path;

        /// Stencil
        Path2 stencil;

        /// Current Font.
        FontPtr font;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha = 1;

        /// Width of a stroked line.
        float stroke_width = 1;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode = BlendMode::SOURCE_OVER;

        /// Shape at the end of a line.
        LineCap line_cap = LineCap::BUTT;

        /// How different line segments are connected.
        LineJoin line_join = LineJoin::MITER;
    };

private:
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

    /// The current State of the Plotter. Is used to diff against the target state.
    struct _InternalState : public PainterState {
        /// Screen size.
        Size2i screen_size = Size2i::zero();

        /// Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        /// How many indices to feed into a patch.
        int patch_vertices = 2;

        /// Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        V2f vec2_aux1;

        /// Index at which the paint buffer is bound.
        uint paint_index = 0;

        /// Index at which the clipping buffer is bound.
        uint clip_index = 0;
    };

    /// A Path identifies a range in the Plotters buffer and associates it with additional information.
    struct _Path {

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

    struct _FillCall {

        /// Index of the Path to draw.
        uint path;

        /// Index of the 2D transformation to apply to the path.
        uint path_xform;

        /// Index of the Stencil to draw.
        uint stencil;

        /// Index of the 2D transformation to apply to the stencil.
        uint stencil_xform;

        /// Index of the Paint in the UniformBuffer.
        uint paint;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha;

        /// Winding order of the drawn shape.
        Orientation winding;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode;
    };
    static_assert(sizeof(_FillCall) == 28);

    struct _StrokeCall {

        /// Index of the Path to draw.
        uint path;

        /// Index of the 2D transformation to apply to the path.
        uint path_xform;

        /// Index of the Stencil to draw.
        uint stencil;

        /// Index of the 2D transformation to apply to the stencil.
        uint stencil_xform;

        /// Index of the Paint in the UniformBuffer.
        uint paint;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha;

        /// Width of the stroke in pixels.
        float width;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode;

        /// Shape at the end of a line.
        LineCap cap = LineCap::_CURRENT;

        /// How different line segments are connected.
        LineJoin join = LineJoin::_CURRENT;
    };
    static_assert(sizeof(_StrokeCall) == 32);

    struct _WriteCall {

        /// Index of the Path to draw.
        uint path;

        /// Index of the Stencil to draw.
        uint stencil;

        /// Index of the 2D transformation to apply to the stencil.
        uint stencil_xform;

        /// Index of the Paint in the UniformBuffer.
        uint paint;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode;
    };
    static_assert(sizeof(_WriteCall) == 24);

    using _DrawCall = std::variant<_StrokeCall, _FillCall, _WriteCall>;
    static_assert(sizeof(_DrawCall) == 36);

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
        FragmentPaint(const Paint& paint, const Path2& stencil, const float stroke_width, const Type type);
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

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Plotter);

    /// Construct a new Plotter.
    /// @param context      GraphicsContext to create the VAO in.
    /// @throws OpenGLError If any of the OpenGL buffers could not be generated.
    Plotter(GraphicsContext& context);

    /// Destructor.
    ~Plotter();

    void start_parsing();

    /// Paints the Design of the given Widget.
    void parse(const PlotterDesign& design);

    void finish_parsing();

private:
    /// The current PainterState.
    const PainterState& _get_state() const {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }
    PainterState& _get_state() { return NOTF_FORWARD_CONST_AS_MUTABLE(_get_state()); }

    /// Store a new fill call.
    void _store_fill();

    /// Store a new stroke call.
    void _store_stroke();

    /// Store a new write call.
    void _store_write(std::string text);

    /// Fill implementation.
    void _render_fill(const _FillCall& call);

    /// Stroke implementation.
    void _render_stroke(const _StrokeCall& call);

    /// Write implementation.
    void _render_text(const _WriteCall& call);

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

    /// All paths.
    std::vector<_Path> m_paths;

    /// Draw Calls.
    std::vector<_DrawCall> m_drawcalls;

    /// State of the next draw call.
    /// Is controlled by a parsed Design and can cheaply be updated as no OpenGL calls are made until the next stroke,
    /// fill or write-call at which the current state is matched against the server state.
    std::vector<PainterState> m_states;

    _InternalState m_server_state;
};

NOTF_CLOSE_NAMESPACE
