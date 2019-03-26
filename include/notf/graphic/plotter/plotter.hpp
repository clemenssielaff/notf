#pragma once

#include <unordered_map>

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

    struct _XformAttribute {
        using type = M3f;
    };

public:
    // internal buffers -------------------------------------------------------

    /// OpenGL buffer holding path vertices.
    using Vertices = vertex_buffer_t<_VertexPosAttribute, _LeftCtrlAttribute, _RightCtrlAttribute>;
    using VerticesPtr = std::shared_ptr<Vertices>;

    /// OpenGL buffer holding path indices.
    using Indices = IndexBuffer<GLuint>;
    using IndicesPtr = std::shared_ptr<Indices>;

    /// OpenGL buffer holding per-instance 2D transformations.
    using XformBuffer = vertex_buffer_t<_XformAttribute>;
    using XformBufferPtr = std::shared_ptr<XformBuffer>;

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
        bool operator==(const Paint& other) const {
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
        bool operator!=(const Paint& other) const { return !operator==(other); }

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

    // enums ------------------------------------------------------------------

    /// Type of cap used at the end of a painted line.
    enum class CapStyle : uchar {
        _CURRENT, // default value, means "use the current one"
        BUTT,
        ROUND,
        SQUARE,
    };

    /// Type of joint beween two painted line segments.
    enum class JointStyle : uchar {
        _CURRENT, // default value, means "use the current one"
        BEVEL,
        ROUND,
        MITER,
    };

    // external draw calls ----------------------------------------------------

    /// State used to contextualize paint operations.
    struct PainterState {

        /// Current Shape;
        Path2Ptr path;

        /// Current Font.
        FontPtr font;

        /// Transformation of the Painter.
        M3f xform = M3f::identity();

        /// Clipping rect (in painter space).
        Aabrf clip;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha = 1;

        /// Width of a stroked line.
        float stroke_width = 1;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode = BlendMode::SOURCE_OVER;

        /// Shape at the end of a line.
        CapStyle line_cap = CapStyle::BUTT;

        /// How different line segments are connected.
        JointStyle joint_style = JointStyle::MITER;

        /// Paint used for painting.
        Paint paint = Color(255, 255, 255);
    };
    static_assert(sizeof(PainterState) == 176);

private:
    /// Type of the patch to draw.
    enum PatchType : int {
        CONVEX = 1,
        CONCAVE = 2,
        TEXT = 3,
        STROKE = 4,
    };

    /// The current State of the Plotter. Is used to diff against the target state.
    /// Default initializes to all invalid values.
    struct InternalState {

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha = -1;

        /// Width of a stroked line.
        float stroke_width = -1;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode = BlendMode::_DEFAULT;

        /// Shape at the end of a line.
        CapStyle cap_style = CapStyle::_CURRENT;

        /// How different line segments are connected.
        JointStyle joint_style = JointStyle::_CURRENT;

        /// Screen size.
        Size2i screen_size = Size2i::zero();

        /// Patch type uniform.
        PatchType patch_type = static_cast<PatchType>(0);

        /// How many indices to feed into a patch.
        int patch_vertices = -1;

        /// Auxiliary vector2 uniform.
        /// Used as the base vertex for shapes and the size of the font atlas for text.
        V2f vec2_aux1 = V2f::zero();

        /// Index at which the paint buffer is bound.
        uint paint_index = 0;

        /// Index at which the xform buffer is bound.
        uint xform_index = 0;

        /// Index at which the clip buffer is bound.
        uint clip_index = 0;
    };

    /// A Path identifies a range in the Plotters buffer and associates it with additional information.
    struct Path {

        /// Offset into the Plotter's vertex buffer.
        /// Is signed because OpenGL does some arithmetic with it and wants to know if it overflowed.
        int vertex_offset;

        /// Offset into the Plotter's index buffer.
        uint index_offset;

        /// Size of this Path in the Plotter's index buffer.
        int size;

        /// Center of this Path in local space.
        V2f center;

        /// Whether this Path is convex or concave.
        bool is_convex;
    };

    // draw calls -------------------------------------------------------------

    struct _CallBase {

        /// Index in `m_paths` of the Path to draw.
        uint path;

        /// Index in `m_xform_buffer` of the 2D transformation to apply to the path.
        uint xform;

        /// Index of the FragmentPaint in `m_paint_buffer`.
        uint paint;

        /// Index in `m_clips` of the Clip to apply to this call.
        uint clip;

        /// Global alpha of the painter, is multiplied on top of the Paint's alpha.
        float alpha;

        /// How the paint is blended with the existing image underneath.
        BlendMode blend_mode;
    };

    struct _FillCall : public _CallBase {};
    struct _WriteCall : public _CallBase {};

    struct _StrokeCall : public _CallBase {

        /// Shape at the end of a line.
        CapStyle cap = CapStyle::_CURRENT;

        /// How different line segments are connected.
        JointStyle join = JointStyle::_CURRENT;

        /// Width of the stroke in pixels.
        float width;
    };

    using DrawCall = std::variant<_StrokeCall, _FillCall, _WriteCall>;

    static_assert(sizeof(_FillCall) == 24);
    static_assert(sizeof(_WriteCall) == 24);
    static_assert(sizeof(_StrokeCall) == 28);
    static_assert(sizeof(DrawCall) == 32);

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
        FragmentPaint(const Paint& paint, const Path2Ptr& stencil, const float stroke_width, const Type type);
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
        Color outer_color = Color::transparent();    // 20 (size = 4)
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

    /// Restores the Plotter into a neutral state before parsing any designs.
    void start_parsing();

    /// Paints the Design of the given Widget.
    /// @param design   Design to parse.
    /// @param xform    Base transformation.
    /// @param clip     Clipping Aabr, in space transformed by `xform`.
    void parse(const PlotterDesign& design, const M3f& xform = M3f::identity(), const Aabrf& clip = Aabrf::wrongest());

    /// Call after parsing the last design.
    /// Uploads all buffers to the GPU and enqueues all draw calls
    void finish_parsing();

private:
    /// The current PainterState.
    const PainterState& _get_state() const {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }
    PainterState& _get_state() { return NOTF_FORWARD_CONST_AS_MUTABLE(_get_state()); }

    /// Pops the current PainterState from the stack.
    void _pop_state() {
        if (m_states.size() > 1) {
            m_states.pop_back();
        } else {
            _get_state() = {};
        }
    }

    /// Stores a path or returns the index of an existing path.
    uint _store_path(const Path2Ptr& path);

    /// Stores all basic call information in the passed call.
    void _store_call_base(_CallBase& call);

    /// Store a new fill call.
    void _store_fill_call();

    /// Store a new stroke call.
    void _store_stroke_call();

    /// Store a new write call.
    void _store_write_call(std::string text);

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
    VerticesPtr m_vertex_buffer;

    /// Indices into the vertex buffer.
    IndicesPtr m_index_buffer;

    /// Buffer containing per-instance transformations.
    XformBufferPtr m_xform_buffer;

    /// Internal vertex object managing attribute- and buffer bindings.
    VertexObjectPtr m_vertex_object;

    /// Uniform buffer containing the paint uniform blocks.
    UniformBufferPtr<FragmentPaint> m_paint_buffer;

    /// All paths, referenced by the drawcalls.
    std::vector<Path> m_paths;

    /// Quick map from an existing Path to an index in `m_paths`.
    std::unordered_map<Path2Ptr, uint> m_path_lookup;

    /// Clips, referenced by the drawcalls
    std::vector<Aabrf> m_clips;

    /// Draw Calls.
    std::vector<DrawCall> m_drawcalls;

    /// State of the next draw call.
    /// Is controlled by a parsed Design and can cheaply be updated as no OpenGL calls are made until the next stroke,
    /// fill or write-call at which the current state is matched against the server state.
    std::vector<PainterState> m_states;

    /// Actual GPU state.
    InternalState m_server_state;
};

NOTF_CLOSE_NAMESPACE
