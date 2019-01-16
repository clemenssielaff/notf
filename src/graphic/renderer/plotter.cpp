/// The Plotter uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
/// of information on to the GPU. There are however a few things to consider when transforming a Bezier curve into the
/// Plotter GPU representation.
///
/// The shader takes a patch that is made up of two vertices v1 and v2.
/// Each vertex has 3 attributes:
///     a1. its position in absolute screen coordinates
///     a2. the modified(*) position of a bezier control point to the left, in screen coordinates relative to a1.
///     a3. the modified(*) position of a bezier control point to the right, in screen coordinates relative to a1.
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

#include "notf/graphic/renderer/plotter.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/bezier.hpp"
#include "notf/common/filesystem.hpp"
#include "notf/common/matrix4.hpp"
#include "notf/common/polygon.hpp"
#include "notf/common/utf8.hpp"
#include "notf/common/variant.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/index_array.hpp"
#include "notf/graphic/shader.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/vertex_array.hpp"

namespace {

NOTF_USING_NAMESPACE;

struct VertexPos : public AttributeTrait {
    NOTF_UNUSED constexpr static uint location = 0;
    using type = V2f;
    using kind = AttributeKind::Position;
};

struct LeftCtrlPos : public AttributeTrait {
    NOTF_UNUSED constexpr static uint location = 1;
    using type = V2f;
};

struct RightCtrlPos : public AttributeTrait {
    NOTF_UNUSED constexpr static uint location = 2;
    using type = V2f;
};

using PlotVertexArray = VertexArray<VertexPos, LeftCtrlPos, RightCtrlPos>;
using PlotIndexArray = IndexArray<GLuint>;

const GLenum g_index_type = to_gl_type(PlotIndexArray::index_t{});

void set_pos(PlotVertexArray::Vertex& vertex, V2f pos) { std::get<0>(vertex) = std::move(pos); }
void set_first_ctrl(PlotVertexArray::Vertex& vertex, V2f pos) { std::get<1>(vertex) = std::move(pos); }
void set_second_ctrl(PlotVertexArray::Vertex& vertex, V2f pos) { std::get<2>(vertex) = std::move(pos); }

void set_modified_first_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& left_segment) {
    const V2f delta = left_segment.ctrl2 - left_segment.end;
    set_first_ctrl(vertex, delta.is_zero() ? -(left_segment.get_tangent(1).normalize()) :
                                             delta.get_normalized() * (delta.get_magnitude() + 1));
}

void set_modified_second_ctrl(PlotVertexArray::Vertex& vertex, const CubicBezier2f::Segment& right_segment) {
    const V2f delta = right_segment.ctrl1 - right_segment.start;
    set_second_ctrl(vertex, delta.is_zero() ? right_segment.get_tangent(0).normalize() :
                                              delta.get_normalized() * (delta.get_magnitude() + 1));
}

} // namespace

NOTF_OPEN_NAMESPACE

// paint ============================================================================================================ //

Plotter::Paint Plotter::Paint::linear_gradient(const V2f& start_pos, const V2f& end_pos, const Color start_color,
                                               const Color end_color) {
    static const float large_number = 1e5;

    V2f delta = end_pos - start_pos;
    const float mag = delta.get_magnitude();
    if (is_approx(mag, 0, 0.0001)) {
        delta.x() = 0;
        delta.y() = 1;
    } else {
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
    paint.extent.width() = large_number;
    paint.extent.height() = large_number + (mag / 2);
    paint.inner_color = std::move(start_color);
    paint.outer_color = std::move(end_color);
    return paint;
}

Plotter::Paint Plotter::Paint::radial_gradient(const V2f& center, const float inner_radius, const float outer_radius,
                                               const Color inner_color, const Color outer_color) {
    Paint paint;
    paint.xform = M3f::translation(center);
    paint.radius = (inner_radius + outer_radius) * 0.5f;
    paint.feather = max(1, outer_radius - inner_radius);
    paint.extent.width() = paint.radius;
    paint.extent.height() = paint.radius;
    paint.inner_color = std::move(inner_color);
    paint.outer_color = std::move(outer_color);
    return paint;
}

Plotter::Paint Plotter::Paint::box_gradient(const V2f& center, const Size2f& extend, const float radius,
                                            const float feather, const Color inner_color, const Color outer_color) {
    Paint paint;
    paint.xform = M3f::translation(V2f{center.x() + extend.width() / 2, center.y() + extend.height() / 2});
    paint.radius = radius;
    paint.feather = max(1, feather);
    paint.extent.width() = extend.width() / 2;
    paint.extent.height() = extend.height() / 2;
    paint.inner_color = std::move(inner_color);
    paint.outer_color = std::move(outer_color);
    return paint;
}

Plotter::Paint Plotter::Paint::texture_pattern(const V2f& origin, const Size2f& extend, TexturePtr texture,
                                               const float angle, const float alpha) {
    Paint paint;
    paint.xform = M3f::rotation(angle);
    paint.xform[2][0] = origin.x();
    paint.xform[2][1] = origin.y();
    paint.extent.width() = extend.width();
    paint.extent.height() = -extend.height();
    paint.texture = texture;
    paint.inner_color = Color(1, 1, 1, alpha);
    paint.outer_color = Color(1, 1, 1, alpha);
    return paint;
}

// plotter ========================================================================================================== //

Plotter::Plotter(GraphicsContext& context) : m_context(context) {
    NOTF_GUARD(m_context.make_current());

    { // pipeline
        const std::string vertex_src = load_file("/home/clemens/code/notf/res/shaders/plotter.vert");
        VertexShaderPtr vertex_shader = VertexShader::create("plotter.vert", vertex_src);

        const std::string ctrl_src = load_file("/home/clemens/code/notf/res/shaders/plotter.tesc");
        const std::string eval_src = load_file("/home/clemens/code/notf/res/shaders/plotter.tese");
        TesselationShaderPtr tess_shader = TesselationShader::create("plotter.tess", ctrl_src, eval_src);

        const std::string frag_src = load_file("/home/clemens/code/notf/res/shaders/plotter.frag");
        FragmentShaderPtr frag_shader = FragmentShader::create("plotter.frag", frag_src);

        m_program = ShaderProgram::create("Plotter", vertex_shader, tess_shader, frag_shader);

        m_program->get_uniform("aa_width").set(static_cast<float>(sqrt(2.l) / 2.l));
        m_program->get_uniform("font_texture").set(TheGraphicsSystem::get_environment().font_atlas_texture_slot);
    }

    // vao
    NOTF_CHECK_GL(glGenVertexArrays(1, &m_vao_id));
    if (!m_vao_id) { NOTF_THROW(OpenGLError, "Failed to allocate the Plotter VAO"); }
    NOTF_GUARD(GraphicsContext::VaoGuard(m_vao_id));

    { // vertices
        VertexArrayType::Args vertex_args;
        vertex_args.usage = GLUsage::STREAM_DRAW;
        m_vertices = std::make_unique<PlotVertexArray>(std::move(vertex_args));
        static_cast<PlotVertexArray*>(m_vertices.get())->init();
    }

    // indices
    m_indices = std::make_unique<PlotIndexArray>();
    static_cast<PlotIndexArray*>(m_indices.get())->init();
}

Plotter::~Plotter() {
    NOTF_GUARD(m_context.make_current());
    NOTF_CHECK_GL(glDeleteVertexArrays(1, &m_vao_id));
    m_indices.reset();
    m_vertices.reset();
    m_program.reset();
}

Plotter::PathPtr Plotter::add(const CubicBezier2f& spline) {
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    // path
    PathPtr path = Path::_create_shared();
    path->offset = narrow_cast<uint>(indices.size());
    path->center = V2f::zero(); // TODO: extract more Plotter::Path information from bezier splines
    path->is_convex = false;    //
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
            set_first_ctrl(vertex, V2f::zero());
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
            set_second_ctrl(vertex, V2f::zero());
            vertices.emplace_back(std::move(vertex));
        }
    }

    return path;
}

Plotter::PathPtr Plotter::add(const Polygonf& polygon) {
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    // path
    PathPtr path = Path::_create_shared();
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
    for (const V2f& point : polygon.get_vertices()) {
        PlotVertexArray::Vertex vertex;
        set_pos(vertex, point);
        set_first_ctrl(vertex, V2f::zero());
        set_second_ctrl(vertex, V2f::zero());
        vertices.emplace_back(std::move(vertex));
    }

    return path;
}

void Plotter::write(const std::string& text, TextInfo info) {
    std::vector<PlotVertexArray::Vertex>& vertices = static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer();
    std::vector<GLuint>& indices = static_cast<PlotIndexArray*>(m_indices.get())->buffer();

    const size_t index_offset = indices.size();
    const GLuint first_index = narrow_cast<GLuint>(vertices.size());

    if (!info.font) {
        NOTF_LOG_WARN("Cannot add text without a font");
        return;
    }

    { // vertices
        const Utf8 utf8_text(text);

        // make sure that text is always rendered on the pixel grid, not between pixels
        const V2f& translation = info.translation;
        Glyph::coord_t x = static_cast<Glyph::coord_t>(roundf(translation.x()));
        Glyph::coord_t y = static_cast<Glyph::coord_t>(roundf(translation.y()));

        for (const auto character : utf8_text) {
            const Glyph& glyph = info.font->glyph(static_cast<codepoint_t>(character));

            if (!glyph.rect.width || !glyph.rect.height) {
                // skip glyphs without pixels
            } else {

                // quad which renders the glyph
                const Aabrf quad((x + glyph.left), (y - glyph.rect.height + glyph.top), glyph.rect.width,
                                 glyph.rect.height);

                // uv coordinates of the glyph - must be divided by the size of the font texture!
                const V2f uv = V2f{glyph.rect.x, glyph.rect.y};

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
    info.path = Path::_create_shared();
    info.path->offset = narrow_cast<uint>(index_offset);
    info.path->size = narrow_cast<int>(indices.size() - index_offset);
    m_drawcall_buffer.emplace_back(DrawCall{std::move(info)});
}

void Plotter::stroke(valid_ptr<PathPtr> path, StrokeInfo info) {
    if (path->size == 0 || info.width <= 0.f) {
        return; // early out
    }

    // a line must be at least a pixel wide to be drawn, to emulate thinner lines lower the alpha
    info.width = max(1, info.width);

    info.path = std::move(path);
    m_drawcall_buffer.emplace_back(DrawCall{std::move(info)});
}

void Plotter::fill(valid_ptr<PathPtr> path, FillInfo info) {
    if (path->size == 0) {
        return; // early out
    }
    info.path = std::move(path);
    m_drawcall_buffer.emplace_back(DrawCall{std::move(info)});
}

void Plotter::swap_buffers() {
    NOTF_ASSERT(m_context.is_current());

    const auto vao_guard = GraphicsContext::VaoGuard(m_vao_id);

    static_cast<PlotVertexArray*>(m_vertices.get())->init();
    static_cast<PlotIndexArray*>(m_indices.get())->init();

    // TODO: combine batches with same type & info -- that's what batches are there for
    std::swap(m_drawcalls, m_drawcall_buffer);
    m_drawcall_buffer.clear();
}

void Plotter::clear() {
    NOTF_ASSERT(m_context.is_current());

    static_cast<PlotVertexArray*>(m_vertices.get())->get_buffer().clear();
    static_cast<PlotIndexArray*>(m_indices.get())->buffer().clear();
    m_drawcall_buffer.clear();
}

void Plotter::render() const {
    NOTF_ASSERT(m_context.is_current());
    if (m_indices->is_empty()) { return; }
    const auto vao_guard = GraphicsContext::VaoGuard(m_vao_id);

    NOTF_CHECK_GL(glEnable(GL_CULL_FACE));
    NOTF_CHECK_GL(glCullFace(GL_BACK));
    NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, m_state.patch_vertices));
    NOTF_CHECK_GL(glEnable(GL_BLEND));
    NOTF_CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    NOTF_GUARD(m_context.bind_program(m_program));

    // screen size
    const Aabri& render_area = m_context.get_render_area();
    if (m_state.screen_size != render_area.get_size()) {
        m_state.screen_size = render_area.get_size();
        m_program->set_uniform("projection", M4f::orthographic(0, m_state.screen_size.width(), 0,
                                                               m_state.screen_size.height(), 0, 2));
    }

    // perform the render calls
    for (const DrawCall& drawcall : m_drawcalls) {
        std::visit(overloaded{
                       [&](const StrokeInfo& stroke) { _render_line(stroke); },
                       [&](const FillInfo& shape) { _render_shape(shape); },
                       [&](const TextInfo& text) { _render_text(text); },
                   },
                   drawcall);
    }
}

void Plotter::_render_line(const StrokeInfo& stroke) const {
    // patch vertices
    if (m_state.patch_vertices != 2) {
        m_state.patch_vertices = 2;
        NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, 2));
    }

    // patch type
    if (m_state.patch_type != PatchType::STROKE) {
        m_program->set_uniform("patch_type", to_number(PatchType::STROKE));
        m_state.patch_type = PatchType::STROKE;
    }

    // stroke width
    if (abs(m_state.stroke_width - stroke.width) > precision_high<float>()) {
        m_program->set_uniform("stroke_width", stroke.width);
        m_state.stroke_width = stroke.width;
    }

    NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(stroke.path->size), g_index_type,
                                 gl_buffer_offset(stroke.path->offset * sizeof(PlotIndexArray::index_t))));
}

void Plotter::_render_shape(const FillInfo& shape) const {

    // patch vertices
    if (m_state.patch_vertices != 2) {
        m_state.patch_vertices = 2;
        NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, 2));
    }

    // patch type
    if (shape.path->is_convex) {
        if (m_state.patch_type != PatchType::CONVEX) {
            m_program->set_uniform("patch_type", to_number(PatchType::CONVEX));
            m_state.patch_type = PatchType::CONVEX;
        }
    } else {
        if (m_state.patch_type != PatchType::CONCAVE) {
            m_program->set_uniform("patch_type", to_number(PatchType::CONCAVE));
            m_state.patch_type = PatchType::CONCAVE;
        }
    }

    // base vertex
    if (!shape.path->center.is_approx(m_state.vec2_aux1)) {
        // with a purely convex polygon, we can safely put the base vertex into the center of the polygon as it
        // will always be inside and it should never fall onto an existing vertex
        // this way, we can use antialiasing at the outer edge
        m_program->set_uniform("vec2_aux1", shape.path->center);
        m_state.vec2_aux1 = shape.path->center;
    }

    if (shape.path->is_convex) {
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(shape.path->size), g_index_type,
                                     gl_buffer_offset(shape.path->offset * sizeof(PlotIndexArray::index_t))));
    }

    // concave
    else {
        // TODO: concave shapes have no antialiasing yet
        // this actually covers both single concave and multiple polygons with holes

        NOTF_CHECK_GL(glEnable(GL_STENCIL_TEST));                           // enable stencil
        NOTF_CHECK_GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)); // do not write into color buffer
        NOTF_CHECK_GL(glStencilMask(0xff));                                 // write to all bits of the stencil buffer
        NOTF_CHECK_GL(glStencilFunc(GL_ALWAYS, 0, 1)); //  Always pass (other parameters do not matter for GL_ALWAYS)

        NOTF_CHECK_GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP));
        NOTF_CHECK_GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP));
        NOTF_CHECK_GL(glDisable(GL_CULL_FACE));
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(shape.path->size), g_index_type,
                                     gl_buffer_offset(shape.path->offset * sizeof(PlotIndexArray::index_t))));
        NOTF_CHECK_GL(glEnable(GL_CULL_FACE));

        NOTF_CHECK_GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
        NOTF_CHECK_GL(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff));          // only write to pixels that are inside the
                                                                        // polygon
        NOTF_CHECK_GL(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than
                                                               // clearing it at the start)

        // render colors here, same area as before if you don't want to clear the stencil buffer every time
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(shape.path->size), g_index_type,
                                     gl_buffer_offset(shape.path->offset * sizeof(PlotIndexArray::index_t))));

        NOTF_CHECK_GL(glDisable(GL_STENCIL_TEST));
    }
}

void Plotter::_render_text(const TextInfo& text) const {

    // patch vertices
    if (m_state.patch_vertices != 1) {
        m_state.patch_vertices = 1;
        NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, 1));
    }

    // patch type
    if (m_state.patch_type != PatchType::TEXT) {
        m_program->set_uniform("patch_type", to_number(PatchType::TEXT));
        m_state.patch_type = PatchType::TEXT;
    }

    const TexturePtr font_atlas = TheGraphicsSystem()->get_font_manager().get_atlas_texture();
    const Size2i& atlas_size = font_atlas->get_size();

    // atlas size
    const V2f atlas_size_vec{atlas_size.width(), atlas_size.height()};
    if (!atlas_size_vec.is_approx(m_state.vec2_aux1)) {
        m_program->set_uniform("vec2_aux1", atlas_size_vec);
        m_state.vec2_aux1 = atlas_size_vec;
    }

    NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(text.path->size), g_index_type,
                                 gl_buffer_offset(text.path->offset * sizeof(PlotIndexArray::index_t))));
}

NOTF_CLOSE_NAMESPACE
