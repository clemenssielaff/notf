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
/// Therefore each control point ax is modified with the following formula (with T begin the tangent normal in the
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

#include "graphics/renderer/plotter.hpp"

#include "common/bezier.hpp"
#include "common/log.hpp"
#include "common/polygon.hpp"
#include "common/system.hpp"
#include "graphics/core/graphics_context.hpp"
#include "graphics/core/index_array.hpp"
#include "graphics/core/pipeline.hpp"
#include "graphics/core/shader.hpp"
#include "graphics/core/texture.hpp"
#include "graphics/core/vertex_array.hpp"
#include "graphics/text/font_manager.hpp"

namespace {

NOTF_USING_NAMESPACE;

struct VertexPos : public AttributeTrait {
    constexpr static uint location = 0;
    using type = Vector2f;
    using kind = AttributeKind::Position;
};

struct LeftCtrlPos : public AttributeTrait {
    constexpr static uint location = 1;
    using type = Vector2f;
};

struct RightCtrlPos : public AttributeTrait {
    constexpr static uint location = 2;
    using type = Vector2f;
};

using PlotVertexArray = VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>;
using PlotIndexArray = IndexArray<GLuint>;

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

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Plotter::Paint Plotter::Paint::linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                               const Color start_color, const Color end_color)
{
    static const float large_number = 1e5;

    Vector2f delta = end_pos - start_pos;
    const float mag = delta.magnitude();
    if (is_approx(mag, 0, 0.0001)) {
        delta.x() = 0;
        delta.y() = 1;
    }
    else {
        delta.x() /= mag;
        delta.y() /= mag;
    }

    Paint paint;
    paint.xform[0][0] = delta.y();
    paint.xform[0][1] = -delta.x();
    paint.xform[1][0] = delta.x();
    paint.xform[1][1] = delta.y();
    paint.xform[2][0] = start_pos.x() - (delta.x() * large_number);
    paint.xform[2][1] = start_pos.y() - (delta.y() * large_number);
    paint.radius = 0.0f;
    paint.feather = max(1, mag);
    paint.extent.width = large_number;
    paint.extent.height = large_number + (mag / 2);
    paint.inner_color = std::move(start_color);
    paint.outer_color = std::move(end_color);
    return paint;
}

Plotter::Paint
Plotter::Paint::radial_gradient(const Vector2f& center, const float inner_radius, const float outer_radius,
                                const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform = Matrix3f::translation(center);
    paint.radius = (inner_radius + outer_radius) * 0.5f;
    paint.feather = max(1, outer_radius - inner_radius);
    paint.extent.width = paint.radius;
    paint.extent.height = paint.radius;
    paint.inner_color = std::move(inner_color);
    paint.outer_color = std::move(outer_color);
    return paint;
}

Plotter::Paint Plotter::Paint::box_gradient(const Vector2f& center, const Size2f& extend, const float radius,
                                            const float feather, const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform = Matrix3f::translation({center.x() + extend.width / 2, center.y() + extend.height / 2});
    paint.radius = radius;
    paint.feather = max(1, feather);
    paint.extent.width = extend.width / 2;
    paint.extent.height = extend.height / 2;
    paint.inner_color = std::move(inner_color);
    paint.outer_color = std::move(outer_color);
    return paint;
}

Plotter::Paint Plotter::Paint::texture_pattern(const Vector2f& origin, const Size2f& extend, TexturePtr texture,
                                               const float angle, const float alpha)
{
    Paint paint;
    paint.xform = Matrix3f::rotation(angle);
    paint.xform[2][0] = origin.x();
    paint.xform[2][1] = origin.y();
    paint.extent.width = extend.width;
    paint.extent.height = -extend.height;
    paint.texture = texture;
    paint.inner_color = Color(1, 1, 1, alpha);
    paint.outer_color = Color(1, 1, 1, alpha);
    return paint;
}

// ================================================================================================================== //

Plotter::Plotter(GraphicsContext& graphics_context)
    : m_graphics_context(graphics_context), m_font_manager(m_graphics_context.get_font_manager())
{
    const GraphicsContext::CurrentGuard current_guard = m_graphics_context.make_current();

    // vao
    notf_check_gl(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) {
        NOTF_THROW(runtime_error, "Failed to allocate the Plotter VAO");
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    { // pipeline
        const std::string vertex_src = load_file("/home/clemens/code/notf/res/shaders/plotter.vert");
        VertexShaderPtr vertex_shader = VertexShader::create(m_graphics_context, "plotter.vert", vertex_src);

        const std::string tess_src = load_file("/home/clemens/code/notf/res/shaders/plotter.tess");
        const std::string eval_src = load_file("/home/clemens/code/notf/res/shaders/plotter.eval");
        TesselationShaderPtr tess_shader
            = TesselationShader::create(m_graphics_context, "plotter.tess", tess_src, eval_src);

        const std::string frag_src = load_file("/home/clemens/code/notf/res/shaders/plotter.frag");
        FragmentShaderPtr frag_shader = FragmentShader::create(m_graphics_context, "plotter.frag", frag_src);

        m_pipeline = Pipeline::create(m_graphics_context, vertex_shader, tess_shader, frag_shader);

        tess_shader->set_uniform("aa_width", 1.5f);
        frag_shader->set_uniform("font_texture", m_graphics_context.get_environment().font_atlas_texture_slot);
    }

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GLUsage::DYNAMIC_DRAW;
        m_vertices = std::make_unique<PlotVertexArray>(std::move(vertex_args));
        static_cast<PlotVertexArray*>(m_vertices.get())->init();
    }

    // indices
    m_indices = std::make_unique<PlotIndexArray>();
    static_cast<PlotIndexArray*>(m_indices.get())->init();
}

Plotter::~Plotter() { notf_check_gl(glDeleteVertexArrays(1, &m_vao_id)); }

Plotter::PathPtr Plotter::add(const CubicBezier2f& spline)
{
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    // path
    PathPtr path = NOTF_MAKE_SHARED_FROM_PRIVATE(Path);
    path->offset = narrow_cast<uint>(indices.size());
    path->center = Vector2f::zero(); // TODO: extract more Plotter::Path information from bezier splines
    path->is_convex = false;         //
    path->is_closed = spline.segments.front().start.is_approx(spline.segments.back().end);

    { // update indices
        indices.reserve(indices.size() + (spline.segments.size() * 4) + 2);

        GLuint index = narrow_cast<GLuint>(vertices.size());

        // start cap
        indices.emplace_back(index);
        indices.emplace_back(index);

        for (size_t i = 0; i < spline.segments.size(); ++i) {
            // segment
            indices.emplace_back(index);
            indices.emplace_back(++index);

            // joint / end cap
            indices.emplace_back(index);
            indices.emplace_back(index);
        }
    }
    path->size = narrow_cast<int>(indices.size() - path->offset);

    { // vertices
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
            const CubicBezier2f::Segment& left_segment = spline.segments[i];
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
    }

    return path;
}

Plotter::PathPtr Plotter::add(const Polygonf& polygon)
{
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    // path
    PathPtr path = NOTF_MAKE_SHARED_FROM_PRIVATE(Path);
    path->offset = narrow_cast<uint>(indices.size());
    path->center = polygon.get_center();
    path->is_convex = polygon.is_convex();
    path->is_closed = true;

    { // update indices
        indices.reserve(indices.size() + (polygon.get_vertex_count() * 2));

        const GLuint first_index = narrow_cast<GLuint>(vertices.size());
        GLuint next_index = first_index;

        for (size_t i = 0; i < polygon.get_vertex_count() - 1; ++i) {
            // first -> (n-1) segment
            indices.emplace_back(next_index++);
            indices.emplace_back(next_index);
        }

        // last segment
        indices.emplace_back(next_index);
        indices.emplace_back(first_index);
    }
    path->size = narrow_cast<int>(indices.size() - path->offset);

    // vertices
    vertices.reserve(vertices.size() + polygon.get_vertex_count());
    for (const Vector2f& point : polygon.get_vertices()) {
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, point);
        set_first_ctrl(vertex, Vector2f::zero());
        set_second_ctrl(vertex, Vector2f::zero());
        vertices.emplace_back(std::move(vertex));
    }

    return path;
}

void Plotter::write(const std::string& text, TextInfo info)
{
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    const size_t index_offset = indices.size();
    const GLuint first_index = narrow_cast<GLuint>(vertices.size());

    if (!info.font) {
        log_warning << "Cannot add text without a font";
        return;
    }

    { // vertices
        const utf8_string utf8_text(text);

        // make sure that text is always rendered on the pixel grid, not between pixels
        const Vector2f& translation = info.translation;
        Glyph::coord_t x = static_cast<Glyph::coord_t>(roundf(translation.x()));
        Glyph::coord_t y = static_cast<Glyph::coord_t>(roundf(translation.y()));

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
                set_first_ctrl(vertex, quad.get_bottom_left());
                set_second_ctrl(vertex, quad.get_top_right());
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

    // draw call
    PathPtr path = NOTF_MAKE_SHARED_FROM_PRIVATE(Path);
    path->offset = narrow_cast<uint>(index_offset);
    path->size = narrow_cast<int>(indices.size() - index_offset);
    m_drawcall_buffer.emplace_back(DrawCall{std::move(info), std::move(path)});
}

void Plotter::stroke(valid_ptr<PathPtr> path, StrokeInfo info)
{
    if (path->size == 0 || info.width <= 0.f) {
        return; // early out
    }

    // a line must be at least a pixel wide to be drawn, to emulate thinner lines lower the alpha
    info.width = max(1, info.width);

    m_drawcall_buffer.emplace_back(DrawCall{std::move(info), std::move(path)});
}

void Plotter::fill(valid_ptr<PathPtr> path, FillInfo info)
{
    if (path->size == 0) {
        return; // early out
    }
    m_drawcall_buffer.emplace_back(DrawCall{std::move(info), std::move(path)});
}

void Plotter::swap_buffers()
{
    const auto vao_guard = VaoBindGuard(m_vao_id);

    static_cast<PlotVertexArray*>(m_vertices.get())->init();
    static_cast<PlotIndexArray*>(m_indices.get())->init();

    // TODO: combine batches with same type & info -- that's what batches are there for
    std::swap(m_drawcalls, m_drawcall_buffer);
    m_drawcall_buffer.clear();
}

void Plotter::clear()
{
    static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer().clear();
    static_cast<PlotIndexArray*>(m_indices.get())->buffer().clear();
    m_drawcall_buffer.clear();
}

void Plotter::render() const
{
    /// Render various draw call types.
    struct DrawCallVisitor {

        // fields ----------------------------------------------------------------------------------------------------//

        /// This plotter.
        const Plotter& plotter;

        /// Draw call target.
        const DrawCall& call;

        // methods ---------------------------------------------------------------------------------------------------//

        /// Draw a stroke.
        void operator()(const StrokeInfo& stroke) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();
            Path& path = *call.path;

            // patch vertices
            if (plotter.m_state.patch_vertices != 2) {
                plotter.m_state.patch_vertices = 2;
                notf_check_gl(glPatchParameteri(GL_PATCH_VERTICES, 2));
            }

            // patch type
            if (plotter.m_state.patch_type != PatchType::STROKE) {
                pipeline.get_tesselation_shader()->set_uniform("patch_type", to_number(PatchType::STROKE));
                plotter.m_state.patch_type = PatchType::STROKE;
            }

            // stroke width
            if (abs(plotter.m_state.stroke_width - stroke.width) > precision_high<float>()) {
                pipeline.get_tesselation_shader()->set_uniform("stroke_width", stroke.width);
                plotter.m_state.stroke_width = stroke.width;
            }

            notf_check_gl(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                         gl_buffer_offset(path.offset * sizeof(PlotIndexArray::index_t))));
        }

        /// Fill a shape.
        void operator()(const FillInfo& /*shape*/) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();
            Path& path = *call.path;

            // patch vertices
            if (plotter.m_state.patch_vertices != 2) {
                plotter.m_state.patch_vertices = 2;
                notf_check_gl(glPatchParameteri(GL_PATCH_VERTICES, 2));
            }

            // patch type
            if (path.is_convex) {
                if (plotter.m_state.patch_type != PatchType::CONVEX) {
                    pipeline.get_tesselation_shader()->set_uniform("patch_type", to_number(PatchType::CONVEX));
                    plotter.m_state.patch_type = PatchType::CONVEX;
                }
            }
            else {
                if (plotter.m_state.patch_type != PatchType::CONCAVE) {
                    pipeline.get_tesselation_shader()->set_uniform("patch_type", to_number(PatchType::CONCAVE));
                    plotter.m_state.patch_type = PatchType::CONCAVE;
                }
            }

            // base vertex
            if (!path.center.is_approx(plotter.m_state.vec2_aux1)) {
                // with a purely convex polygon, we can safely put the base vertex into the center of the polygon as it
                // will always be inside and it should never fall onto an existing vertex
                // this way, we can use antialiasing at the outer edge
                pipeline.get_tesselation_shader()->set_uniform("vec2_aux1", path.center);
                plotter.m_state.vec2_aux1 = path.center;
            }

            if (path.is_convex) {
                notf_check_gl(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                             gl_buffer_offset(path.offset * sizeof(PlotIndexArray::index_t))));
            }

            // concave
            else {
                // TODO: concave shapes have no antialiasing yet
                // TODO: this actually covers both single concave and multiple polygons with holes

                notf_check_gl(glEnable(GL_STENCIL_TEST));                           // enable stencil
                notf_check_gl(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)); // do not write into color buffer
                notf_check_gl(glStencilMask(0xff));            // write to all bits of the stencil buffer
                notf_check_gl(glStencilFunc(GL_ALWAYS, 0, 1)); //  Always pass (other values are default values and do
                                                               //  not matter for GL_ALWAYS)

                notf_check_gl(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP));
                notf_check_gl(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP));
                notf_check_gl(glDisable(GL_CULL_FACE));
                notf_check_gl(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                             gl_buffer_offset(path.offset * sizeof(PlotIndexArray::index_t))));
                notf_check_gl(glEnable(GL_CULL_FACE));

                notf_check_gl(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
                notf_check_gl(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff)); // only write to pixels that are inside the
                                                                       // polygon
                notf_check_gl(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than
                                                                       // clearing it at the start)

                // render colors here, same area as before if you don't want to clear the stencil buffer every time
                notf_check_gl(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                             gl_buffer_offset(path.offset * sizeof(PlotIndexArray::index_t))));

                notf_check_gl(glDisable(GL_STENCIL_TEST));
            }
        }

        /// Render text.
        void operator()(const TextInfo& /*text*/) const
        {
            Pipeline& pipeline = *plotter.m_pipeline.get();
            Path& path = *call.path;

            // patch vertices
            if (plotter.m_state.patch_vertices != 1) {
                plotter.m_state.patch_vertices = 1;
                notf_check_gl(glPatchParameteri(GL_PATCH_VERTICES, 1));
            }

            // patch type
            if (plotter.m_state.patch_type != PatchType::TEXT) {
                pipeline.get_tesselation_shader()->set_uniform("patch_type", to_number(PatchType::TEXT));
                plotter.m_state.patch_type = PatchType::TEXT;
            }

            const TexturePtr font_atlas = plotter.m_font_manager.get_atlas_texture();
            const Size2i& atlas_size = font_atlas->get_size();

            // atlas size
            const Vector2f atlas_size_vec{atlas_size.width, atlas_size.height};
            if (!atlas_size_vec.is_approx(plotter.m_state.vec2_aux1)) {
                pipeline.get_tesselation_shader()->set_uniform("vec2_aux1", atlas_size_vec);
                plotter.m_state.vec2_aux1 = atlas_size_vec;
            }

            notf_check_gl(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                         gl_buffer_offset(path.offset * sizeof(PlotIndexArray::index_t))));
        }
    };

    // function begins here ========================================================================================= //

    if (m_indices->is_empty()) {
        return;
    }
    const auto vao_guard = VaoBindGuard(m_vao_id);

    notf_check_gl(glEnable(GL_CULL_FACE));
    notf_check_gl(glCullFace(GL_BACK));
    notf_check_gl(glPatchParameteri(GL_PATCH_VERTICES, m_state.patch_vertices));
    notf_check_gl(glEnable(GL_BLEND));
    notf_check_gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    auto pipeline_guard = m_graphics_context.bind_pipeline(m_pipeline);
    const TesselationShaderPtr& tess_shader = m_pipeline->get_tesselation_shader();

    // screen size
    const Aabri& render_area = m_graphics_context.get_render_area();
    if (m_state.screen_size != render_area.get_size()) {
        m_state.screen_size = render_area.get_size();
        tess_shader->set_uniform("projection", Matrix4f::orthographic(0, m_state.screen_size.width, 0,
                                                                      m_state.screen_size.height, 0, 2));
    }

    for (const DrawCall& drawcall : m_drawcalls) {
        notf::visit(DrawCallVisitor{*this, drawcall}, drawcall.info);
    }
}

NOTF_CLOSE_NAMESPACE
