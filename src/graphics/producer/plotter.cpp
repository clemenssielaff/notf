/// The Plotter uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
/// of information on to the GPU. There are however a few things to consider when transforming a Bezier curve into the
/// Plotter GPU representation.
///
/// The shader takes a patch that is made up of two vertices v1 and v2.
/// Each vertex has 3 attributes:
///     a1. its position in absolute screen coordinates
///     a2. the modified(*) position of a bezier control point to the left, in screen coordinates relative to a1.
///     a3. the modified(*) position of a bezier control point to the right, in screen coodinates relative to a1.
///
/// When drawing the spline from a patch of two vertices, only the middle 4 attributes are used:
///
///     v1.a1       is the start point of the bezier spline.
///     v1 + v1.a3  is the first control point
///     v2 + v2.a2  is the second control point
///     v2.a1       is the end point
///
/// (*)
/// For the correct calculation of caps and joints, we need the tangent directions at each vertex.
/// This information is easy to derive if a2 != a1 and a3 != a1. If however, one of the two control points has zero
/// distance to the vertex, the shader would require the next patch in order to get the tangent - which doesn't work.
/// Therefore each control point is modified with the following formula (with T begin the tangent normal in the
/// direction of ax):
///
///     if ax - a1 == (0, 0):
///         ax' = T
///     else:
///         ax' = T * (||ax - a1|| + 1)     (whereby T = |ax - a1|)
///
/// Caps
/// ----
///
/// Without caps, lines would only be antialiased on either sides of the line, not their end points.
/// In order to tell the shader to render a start- or end-cap, we pass two vertices with special requirements:
///
/// For the start cap:
///
///     v1.a1 == v2.a1 && v2.a2 == (0,0)
///
/// For the end cap:
///
///     v1.a1 == v2.a1 && v1.a3 == (0,0)
///
/// If the tangent at the cap is required, you can simply invert the tangent obtained from the other ctrl point.
///
/// Joints
/// ------
///
/// In order to render multiple segments without a visual break, the Plotter adds intermediary joints.
/// A joint segment also consists of two vertices but imposes additional requirements:
///
///     v1.a1 == v2.a1
///
/// This is easily accomplished by re-using indices of the existing vertices and doesn't increase the size of the
/// vertex array.
///
/// Text
/// ====
///
/// We should be able to render a glyph using a single vertex, because we have 6 vertex attributes available to us,
/// and we should always have a 1:1 correspondence from screen to texture pixels:
///
/// 0           | Screen position of the glyph's lower left corner
/// 1           |
/// 2       | Texture coordinate of the glyph's lower left vertex
/// 3       |
/// 4   | Height and with of the glyph in pixels, used both for defining the position of the glyph's upper right corner
/// 5   | as well as its texture coordinate
///
/// Glyphs require Font Atlas size as input (maybe where the shape gets the center vertex?)
///

#include "graphics/producer/plotter.hpp"

#include "common/bezier.hpp"
#include "common/enum.hpp"
#include "common/log.hpp"
#include "common/matrix4.hpp"
#include "common/polygon.hpp"
#include "common/utf.hpp"
#include "common/vector.hpp"
#include "common/vector2.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/index_array.hpp"
#include "graphics/core/opengl.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/core/vertex_array.hpp"
#include "graphics/text/font_manager.hpp"
#include "utils/narrow_cast.hpp"

namespace {

using namespace notf;

struct VertexPos : public AttributeTrait {
    constexpr static uint location = 0;
    using type                     = Vector2f;
    using kind                     = AttributeKind::Position;
};

struct LeftCtrlPos : public AttributeTrait {
    constexpr static uint location = 1;
    using type                     = Vector2f;
};

struct RightCtrlPos : public AttributeTrait {
    constexpr static uint location = 2;
    using type                     = Vector2f;
};

using PlotVertexArray = VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>;
using PlotIndexArray  = IndexArray<GLuint>;

const GLenum g_index_type = to_gl_type(PlotIndexArray::index_t{});

void set_pos(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<0>(vertex) = std::move(pos); }
void set_first_ctrl(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<1>(vertex) = std::move(pos); }
void set_second_ctrl(PlotVertexArray::Vertex& vertex, Vector2f pos) { std::get<2>(vertex) = std::move(pos); }

void set_modified_first_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& left_segment)
{
    const Vector2f delta = left_segment.ctrl2 - left_segment.end;
    set_first_ctrl(vertex, delta.is_zero() ? -(left_segment.tangent(1).normalize()) :
                                             delta.normalize() * (delta.magnitude() + 1));
}

void set_modified_second_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& right_segment)
{
    const Vector2f delta = right_segment.ctrl1 - right_segment.start;
    set_second_ctrl(vertex, delta.is_zero() ? (right_segment.tangent(0).normalize()) :
                                              delta.normalize() * (delta.magnitude() + 1));
}

} // namespace

namespace notf {

Plotter::Plotter(GraphicsContextPtr& context, FontManagerPtr& font_manager)
    : m_graphics_context(*context)
    , m_font_manager(*font_manager)
    , m_pipeline()
    , m_vao_id(0)
    , m_vertices()
    , m_indices()
    , m_batches()
    , m_batch_buffer()
    , m_state()
{
    // vao
    gl_check(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) {
        notf_throw(runtime_error, "Failed to allocate the Plotter VAO");
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    { // pipeline
        const std::string vertex_src  = load_file("/home/clemens/code/notf/res/shaders/plotter.vert");
        VertexShaderPtr vertex_shader = VertexShader::create(context, "plotter.vert", vertex_src);

        const std::string tess_src       = load_file("/home/clemens/code/notf/res/shaders/plotter.tess");
        const std::string eval_src       = load_file("/home/clemens/code/notf/res/shaders/plotter.eval");
        TesselationShaderPtr tess_shader = TesselationShader::create(context, "plotter.tess", tess_src, eval_src);

        const std::string frag_src    = load_file("/home/clemens/code/notf/res/shaders/plotter.frag");
        FragmentShaderPtr frag_shader = FragmentShader::create(context, "plotter.frag", frag_src);

        m_pipeline = Pipeline::create(context, vertex_shader, tess_shader, frag_shader);

        tess_shader->set_uniform("aa_width", 1.5f);
        frag_shader->set_uniform("font_texture", m_graphics_context.environment().font_atlas_texture_slot);
    }

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GL_DYNAMIC_DRAW;
        m_vertices        = std::make_unique<PlotVertexArray>(std::move(vertex_args));
        static_cast<PlotVertexArray*>(m_vertices.get())->init();
    }

    // indices
    m_indices = std::make_unique<PlotIndexArray>();
    static_cast<PlotIndexArray*>(m_indices.get())->init();
}

Plotter::~Plotter() { gl_check(glDeleteVertexArrays(1, &m_vao_id)); }

void Plotter::apply()
{
    const auto vao_guard = VaoBindGuard(m_vao_id);

    static_cast<PlotVertexArray*>(m_vertices.get())->init();
    static_cast<PlotIndexArray*>(m_indices.get())->init();

    // TODO: combine batches with same type & info -- that's what batches are there for
    std::swap(m_batches, m_batch_buffer);
    m_batch_buffer.clear();
}

void Plotter::clear()
{
    static_cast<PlotVertexArray*>(m_vertices.get())->buffer().clear();
    static_cast<PlotIndexArray*>(m_indices.get())->buffer().clear();
    m_batch_buffer.clear();
}

void Plotter::render() const
{
    /// Render various batch types.
    struct GraphicsProducer {

        // fields ----------------------------------------------------------------------------------------------------//

        /// This plotter.
        const Plotter& plotter;

        /// Batch target.
        const Batch& batch;

        // methods ---------------------------------------------------------------------------------------------------//

        /// Draw a stroke.
        void operator()(const StrokeInfo& stroke) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();

            // patch vertices
            if (plotter.m_state.patch_vertices != 2) {
                plotter.m_state.patch_vertices = 2;
                gl_check(glPatchParameteri(GL_PATCH_VERTICES, 2));
            }

            // patch type
            if (plotter.m_state.patch_type != PatchType::STROKE) {
                pipeline.tesselation_shader()->set_uniform("patch_type", to_number(PatchType::STROKE));
                plotter.m_state.patch_type = PatchType::STROKE;
            }

            // stroke width
            if (abs(plotter.m_state.stroke_width - stroke.width) > precision_high<float>()) {
                pipeline.tesselation_shader()->set_uniform("stroke_width", stroke.width);
                plotter.m_state.stroke_width = stroke.width;
            }

            gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                    gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));
        }

        /// Draw a shape.
        void operator()(const ShapeInfo& shape) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();

            // patch vertices
            if (plotter.m_state.patch_vertices != 2) {
                plotter.m_state.patch_vertices = 2;
                gl_check(glPatchParameteri(GL_PATCH_VERTICES, 2));
            }

            // patch type
            if (shape.is_convex) {
                if (plotter.m_state.patch_type != PatchType::CONVEX) {
                    pipeline.tesselation_shader()->set_uniform("patch_type", to_number(PatchType::CONVEX));
                    plotter.m_state.patch_type = PatchType::CONVEX;
                }
            }
            else {
                if (plotter.m_state.patch_type != PatchType::CONCAVE) {
                    pipeline.tesselation_shader()->set_uniform("patch_type", to_number(PatchType::CONCAVE));
                    plotter.m_state.patch_type = PatchType::CONCAVE;
                }
            }

            // base vertex
            if (!shape.center.is_approx(plotter.m_state.vec2_aux1)) {
                // with a purely convex polygon, we can safely put the base vertex into the center of the polygon as it
                // will always be inside and it should never fall onto an existing vertex
                // this way, we can use antialiasing at the outer edge
                pipeline.tesselation_shader()->set_uniform("vec2_aux1", shape.center);
                plotter.m_state.vec2_aux1 = shape.center;
            }

            if (shape.is_convex) {
                gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                        gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));
            }

            // concave
            else {
                // TODO: concave shapes have no antialiasing yet
                // TODO: this actually covers both single concave and multiple polygons with holes

                gl_check(glEnable(GL_STENCIL_TEST));                           // enable stencil
                gl_check(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)); // do not write into color buffer
                gl_check(glStencilMask(0xff));            // write to all bits of the stencil buffer
                gl_check(glStencilFunc(GL_ALWAYS, 0, 1)); //  Always pass (other values are default values and do not
                                                          //  matter for GL_ALWAYS)

                gl_check(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP));
                gl_check(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP));
                gl_check(glDisable(GL_CULL_FACE));
                gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                        gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));
                gl_check(glEnable(GL_CULL_FACE));

                gl_check(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
                gl_check(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff)); // only write to pixels that are inside the polygon
                gl_check(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than
                                                                  // clearing it at the start)

                // render colors here, same area as before if you don't want to clear the stencil buffer every time
                gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                        gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));

                gl_check(glDisable(GL_STENCIL_TEST));
            }
        }

        /// Render text.
        void operator()(const TextInfo& /*text*/) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();

            // patch vertices
            if (plotter.m_state.patch_vertices != 1) {
                plotter.m_state.patch_vertices = 1;
                gl_check(glPatchParameteri(GL_PATCH_VERTICES, 1));
            }

            // patch type
            if (plotter.m_state.patch_type != PatchType::TEXT) {
                pipeline.tesselation_shader()->set_uniform("patch_type", to_number(PatchType::TEXT));
                plotter.m_state.patch_type = PatchType::TEXT;
            }

            const TexturePtr& font_atlas = plotter.m_font_manager.atlas_texture();
            const Size2i& atlas_size     = font_atlas->size();

            // atlas size
            const Vector2f atlas_size_vec{atlas_size.width, atlas_size.height};
            if (!atlas_size_vec.is_approx(plotter.m_state.vec2_aux1)) {
                pipeline.tesselation_shader()->set_uniform("vec2_aux1", atlas_size_vec);
                plotter.m_state.vec2_aux1 = atlas_size_vec;
            }

            gl_check(glDrawElements(GL_PATCHES, static_cast<GLsizei>(batch.size), g_index_type,
                                    gl_buffer_offset(batch.offset * sizeof(PlotIndexArray::index_t))));
        }
    };

    // function begins here ////////////////////////////////////////////////////////////////////////////////////////////

    if (m_indices->is_empty()) {
        return;
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    gl_check(glEnable(GL_CULL_FACE));
    gl_check(glCullFace(GL_BACK));
    gl_check(glPatchParameteri(GL_PATCH_VERTICES, m_state.patch_vertices));
    gl_check(glEnable(GL_BLEND));
    gl_check(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_graphics_context.bind_pipeline(m_pipeline);
    const TesselationShaderPtr& tess_shader = m_pipeline->tesselation_shader();

    // screen size
    const Size2i screen_size = {800, 800}; // TODO: get the window size from the graphics context?
    if (m_state.screen_size != screen_size) {
        tess_shader->set_uniform("projection",
                                 Matrix4f::orthographic(0, screen_size.width, 0, screen_size.height, 0, 2));
        m_state.screen_size = screen_size;
    }

    for (const Batch& batch : m_batches) {
        std::visit(GraphicsProducer{*this, batch}, batch.info);
    }

    m_graphics_context.unbind_pipeline();
}

void Plotter::add_stroke(StrokeInfo info, const CubicBezier2f& spline)
{
    if (spline.segments.empty() || info.width <= 0.f) {
        return; // early out
    }

    // TODO: stroke_width less than 1 should set a uniform that fades the line out

    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->buffer();
    std::vector<GLuint>& indices                   = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    const size_t index_offset = indices.size();

    // TODO: differentiate between closed and open splines

    { // update indices
        indices.reserve(indices.size() + (spline.segments.size() * 4) + 2);

        GLuint next_index = narrow_cast<GLuint>(vertices.size());

        // start cap
        indices.emplace_back(next_index);
        indices.emplace_back(next_index);

        for (size_t i = 0; i < spline.segments.size(); ++i) {
            // first -> (n-1) segment
            indices.emplace_back(next_index++);
            indices.emplace_back(next_index);

            // joint / end cap
            indices.emplace_back(next_index);
            indices.emplace_back(next_index);
        }
    }

    vertices.reserve(vertices.size() + spline.segments.size() + 1);

    { // first vertex
        const CubicBezier2f::Segment& first_segment = spline.segments.front();
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, first_segment.start);
        set_first_ctrl(vertex, Vector2f::zero());
        set_modified_second_ctrl(vertex, first_segment);
        vertices.emplace_back(std::move(vertex));
    }

    // middle vertices
    for (size_t i = 0; i < spline.segments.size() - 1; ++i) {
        const CubicBezier2f::Segment& left_segment  = spline.segments[i];
        const CubicBezier2f::Segment& right_segment = spline.segments[i + 1];
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, left_segment.end);
        set_modified_first_ctrl(vertex, left_segment);
        set_modified_second_ctrl(vertex, right_segment);
        vertices.emplace_back(std::move(vertex));
    }

    { // last vertex
        const CubicBezier2f::Segment& last_segment = spline.segments.back();
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, last_segment.end);
        set_modified_first_ctrl(vertex, last_segment);
        set_second_ctrl(vertex, Vector2f::zero());
        vertices.emplace_back(std::move(vertex));
    }

    // batch
    m_batch_buffer.emplace_back(
        Batch{std::move(info), narrow_cast<uint>(index_offset), narrow_cast<int>(indices.size() - index_offset)});
}

void Plotter::add_shape(ShapeInfo info, const Polygonf& polygon)
{
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->buffer();
    std::vector<GLuint>& indices                   = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    const size_t index_offset = indices.size();

    { // update indices
        indices.reserve(indices.size() + (polygon.vertices.size() * 2));

        const GLuint first_index = narrow_cast<GLuint>(vertices.size());
        GLuint next_index        = first_index;

        for (size_t i = 0; i < polygon.vertices.size() - 1; ++i) {
            // first -> (n-1) segment
            indices.emplace_back(next_index++);
            indices.emplace_back(next_index);
        }

        // last segment
        indices.emplace_back(next_index);
        indices.emplace_back(first_index);
    }

    // vertices
    vertices.reserve(vertices.size() + polygon.vertices.size());
    for (const auto& point : polygon.vertices) {
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, point);
        set_first_ctrl(vertex, Vector2f::zero());
        set_second_ctrl(vertex, Vector2f::zero());
        vertices.emplace_back(std::move(vertex));
    }

    // batch
    info.is_convex = polygon.is_convex();
    info.center    = polygon.center();
    m_batch_buffer.emplace_back(
        Batch{std::move(info), narrow_cast<uint>(index_offset), narrow_cast<int>(indices.size() - index_offset)});
}

void Plotter::add_text(TextInfo info, const std::string& text)
{
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->buffer();
    std::vector<GLuint>& indices                   = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    const size_t index_offset = indices.size();
    const GLuint first_index  = narrow_cast<GLuint>(vertices.size());

    if (!info.font) {
        log_warning << "Cannot add text without a font";
        return;
    }

    { // vertices
        const utf8_string utf8_text(text);

        // make sure that text is always rendered on the pixel grid, not between pixels
        const Vector2f& translation = info.translation;
        Glyph::coord_t x            = static_cast<Glyph::coord_t>(roundf(translation.x()));
        Glyph::coord_t y            = static_cast<Glyph::coord_t>(roundf(translation.y()));

        for (const auto character : utf8_text) {
            const Glyph& glyph = info.font->glyph(static_cast<codepoint_t>(character));

            if (!glyph.rect.width || !glyph.rect.height) {
                // skip glyphs without pixels
            }
            else {

                // quad which renders the glyph
                const Aabrf quad((x + glyph.left), (y - glyph.rect.height + glyph.top), glyph.rect.width,
                                 glyph.rect.height);

                // uv coordinates of the glyph - must be divided by the size of the font texture!
                const Vector2f uv = Vector2f{glyph.rect.x, glyph.rect.y};

                // create the vertex
                PlotVertexArray::Vertex vertex;
                set_pos(vertex, uv);
                set_first_ctrl(vertex, quad.bottom_left());
                set_second_ctrl(vertex, quad.top_right());
                vertices.emplace_back(std::move(vertex));
            }

            // advance to the next character position
            x += glyph.advance_x;
            y += glyph.advance_y;
        }
    }

    // indices
    indices.reserve(indices.size() + (vertices.size() - first_index));
    for (GLuint i = first_index, end = narrow_cast<GLuint>(vertices.size()); i < end; ++i) {
        indices.emplace_back(i);
    }

    // batch
    m_batch_buffer.emplace_back(
        Batch{std::move(info), narrow_cast<uint>(index_offset), narrow_cast<int>(indices.size() - index_offset)});
}

} // namespace notf
