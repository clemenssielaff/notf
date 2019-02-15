/// The Plotter uses OpenGL shader tesselation for most of the primitive construction, it only passes the bare minimum
/// of information on to the GPU required to tesselate a cubic bezier spline. There are however a few things to consider
/// when transforming a Bezier curve into the Plotter GPU representation.
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
/// It is possible to render a glyph using a single vertex with the given 6 vertex attributes available, because there
/// is a 1:1 correspondence from screen to texture pixels:
///
/// 0           | Screen position of the glyph's lower left corner
/// 1           |
/// 2       | Texture coordinate of the glyph's lower left vertex
/// 3       |
/// 4   | Height and with of the glyph in pixels, used both for defining the position of the glyph's upper right corner
/// 5   | as well as its texture coordinate
///
/// In order for Glyphs to render, the Shader requires the FontAtlas size, which is passed in to the same uniform as is
/// used as the "center" vertex for shapes.

#include "notf/graphic/renderer/plotter.hpp"

#include "notf/common/filesystem.hpp"
#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/polygon.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/uniform_buffer.hpp"

NOTF_USING_NAMESPACE;

namespace {

using UsageHint = notf::detail::AnyOpenGLBuffer::UsageHint;

static constexpr GLenum g_index_type = to_gl_type(Plotter::IndexBuffer::index_t{});

/// simple convenience methods to make the code more readable

void set_pos(Plotter::VertexBuffer::vertex_t& vertex, V2f pos) { std::get<0>(vertex) = std::move(pos); }

void set_first_ctrl(Plotter::VertexBuffer::vertex_t& vertex, V2f pos) { std::get<1>(vertex) = std::move(pos); }

void set_second_ctrl(Plotter::VertexBuffer::vertex_t& vertex, V2f pos) { std::get<2>(vertex) = std::move(pos); }

/// The `set_modified_...~ methods modify the given ctrl point so we can be certain that is stores the outgoing tangent
/// of the vertex. For that, we get the tangent either from the ctrl point itself (if the distance to the vertex > 0) or
/// by calculating the tangent from the spline at the vertex. The control point is then moved onto the tangent,
/// one unit further away from the vertex than originally. If we encounter a ctrl point in the shader that is of unit
/// length, we know that in reality it was positioned on the vertex itself and still get the vertex tangent.

void set_modified_first_ctrl(Plotter::VertexBuffer::vertex_t& vertex, const CubicBezier2f::Segment& left_segment) {
    const V2f delta = left_segment.ctrl2 - left_segment.end;
    set_first_ctrl(vertex, delta.is_zero() ? -(left_segment.get_tangent(1).normalize()) :
                                             delta.get_normalized() * (delta.get_magnitude() + 1));
}

void set_modified_second_ctrl(Plotter::VertexBuffer::vertex_t& vertex, const CubicBezier2f::Segment& right_segment) {
    const V2f delta = right_segment.ctrl1 - right_segment.start;
    set_second_ctrl(vertex, delta.is_zero() ? right_segment.get_tangent(0).normalize() :
                                              delta.get_normalized() * (delta.get_magnitude() + 1));
}

} // namespace

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
    paint.gradient_radius = 0.0f;
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
    paint.gradient_radius = (inner_radius + outer_radius) * 0.5f;
    paint.feather = max(1, outer_radius - inner_radius);
    paint.extent.width() = paint.gradient_radius;
    paint.extent.height() = paint.gradient_radius;
    paint.inner_color = std::move(inner_color);
    paint.outer_color = std::move(outer_color);
    return paint;
}

Plotter::Paint Plotter::Paint::box_gradient(const V2f& center, const Size2f& extend, const float radius,
                                            const float feather, const Color inner_color, const Color outer_color) {
    Paint paint;
    paint.xform = M3f::translation(V2f{center.x() + extend.width() / 2, center.y() + extend.height() / 2});
    paint.gradient_radius = radius;
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

// fragment paint =================================================================================================== //

Plotter::FragmentPaint::FragmentPaint(const Paint& paint, const Clipping& clipping, const float stroke_width,
                                      const Type type) {
    // paint
    const M3f paint_xform = paint.xform.get_inverse();
    paint_rotation[0] = paint_xform[0][0];
    paint_rotation[1] = paint_xform[0][1];
    paint_rotation[2] = paint_xform[1][0];
    paint_rotation[3] = paint_xform[1][1];
    paint_translation[0] = paint_xform[2][0];
    paint_translation[1] = paint_xform[2][1];
    paint_size[0] = paint.extent.width();
    paint_size[1] = paint.extent.height();

    // clipping
    if (clipping.size.is_valid()) {
        clip_rotation[0] = clipping.xform[0][0];
        clip_rotation[1] = clipping.xform[0][1];
        clip_rotation[2] = clipping.xform[1][0];
        clip_rotation[3] = clipping.xform[1][1];
        clip_translation[0] = clipping.xform[2][0];
        clip_translation[1] = clipping.xform[2][1];
        clip_size[0] = clipping.size.width() / 2;
        clip_size[1] = clipping.size.height() / 2;
    }

    // color
    inner_color = paint.inner_color.premultiplied();
    outer_color = paint.outer_color.premultiplied();

    // type
    if (type == Type::GRADIENT && paint.texture) {
        this->type = Type::IMAGE;
    } else {
        this->type = type;
    }

    this->stroke_width = stroke_width;
    gradient_radius = paint.gradient_radius;
    feather = paint.feather;
}

// plotter ========================================================================================================== //

Plotter::Plotter(GraphicsContext& context) : m_context(context) {

    NOTF_GUARD(m_context.make_current());

    { // program
        const std::string vertex_src = load_file("/home/clemens/code/notf/res/shaders/plotter.vert");
        VertexShaderPtr vertex_shader = VertexShader::create("plotter.vert", vertex_src);

        const std::string ctrl_src = load_file("/home/clemens/code/notf/res/shaders/plotter.tesc");
        const std::string eval_src = load_file("/home/clemens/code/notf/res/shaders/plotter.tese");
        TesselationShaderPtr tess_shader = TesselationShader::create("plotter.tess", ctrl_src, eval_src);

        const std::string frag_src = load_file("/home/clemens/code/notf/res/shaders/plotter.frag");
        FragmentShaderPtr frag_shader = FragmentShader::create("plotter.frag", frag_src);

        m_program = ShaderProgram::create(m_context, "Plotter", vertex_shader, tess_shader, frag_shader);

        m_program->get_uniform("aa_width").set(static_cast<float>(sqrt(2.l) / 2.l));
        m_program->get_uniform("font_texture").set(TheGraphicsSystem::get_environment().font_atlas_texture_slot);
    }

    { // vertex object
        m_vertex_buffer = VertexBuffer::create("PlotterVertexBuffer", UsageHint::STREAM_DRAW);
        m_index_buffer = IndexBuffer::create("PlotterIndexBuffer", UsageHint::STREAM_DRAW);
        m_instance_buffer
            = InstanceBuffer::create("PlotterInstanceBuffer", UsageHint::STREAM_DRAW, /*is_per_instance= */ true);
        m_vertex_object = VertexObject::create(m_context, "PlotterVertexObject");
        m_vertex_object->bind(m_vertex_buffer, 0, 1, 2);
        m_vertex_object->bind(m_index_buffer);
        m_vertex_object->bind(m_instance_buffer, 3);
    }

    { // uniform buffers
        m_paint_buffer = UniformBuffer<FragmentPaint>::create("PlotterPaintBuffer", UsageHint::STREAM_DRAW);
        m_clipping_buffer = UniformBuffer<Clipping>::create("PlotterClippingBuffer", UsageHint::STREAM_DRAW);
    }
}

Plotter::~Plotter() {
    NOTF_GUARD(m_context.make_current());
    m_program.reset();
    m_vertex_buffer.reset();
    m_index_buffer.reset();
    m_instance_buffer.reset();
    m_vertex_object.reset();
    m_paint_buffer.reset();
    m_clipping_buffer.reset();
}

void Plotter::set_shape(const CubicBezier2f& spline) {
    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

    // path
    Path path;
    path.offset = narrow_cast<uint>(indices.size());
    path.center = V2f::zero(); // TODO: extract more Plotter::Path information from bezier splines
    path.is_convex = false;    //
    path.is_closed = spline.segments.front().start.is_approx(spline.segments.back().end);

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
    path.size = narrow_cast<int>(indices.size() - path.offset);

    { // vertices
        vertices.reserve(vertices.size() + spline.segments.size() + 1);

        { // first vertex
            const CubicBezier2f::Segment& first_segment = spline.segments.front();
            VertexBuffer::vertex_t vertex;
            set_pos(vertex, first_segment.start);
            set_first_ctrl(vertex, V2f::zero());
            set_modified_second_ctrl(vertex, first_segment);
            vertices.emplace_back(std::move(vertex));
        }

        // middle vertices
        for (size_t i = 0; i < spline.segments.size() - 1; ++i) {
            const CubicBezier2f::Segment& left_segment = spline.segments[i];
            const CubicBezier2f::Segment& right_segment = spline.segments[i + 1];
            VertexBuffer::vertex_t vertex;
            set_pos(vertex, left_segment.end);
            set_modified_first_ctrl(vertex, left_segment);
            set_modified_second_ctrl(vertex, right_segment);
            vertices.emplace_back(std::move(vertex));
        }

        { // last vertex
            const CubicBezier2f::Segment& last_segment = spline.segments.back();
            VertexBuffer::vertex_t vertex;
            set_pos(vertex, last_segment.end);
            set_modified_first_ctrl(vertex, last_segment);
            set_second_ctrl(vertex, V2f::zero());
            vertices.emplace_back(std::move(vertex));
        }
    }
}

void Plotter::set_shape(const Polygonf& polygon) {

    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

    // path
    Path path;
    path.offset = narrow_cast<uint>(indices.size());
    path.center = polygon.get_center();
    path.is_convex = polygon.is_convex();
    path.is_closed = true;

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
    path.size = narrow_cast<int>(indices.size() - path.offset);

    // vertices
    vertices.reserve(vertices.size() + polygon.get_vertex_count());
    for (const V2f& point : polygon.get_vertices()) {
        VertexBuffer::vertex_t vertex;
        set_pos(vertex, point);
        set_first_ctrl(vertex, V2f::zero());
        set_second_ctrl(vertex, V2f::zero());
        vertices.emplace_back(std::move(vertex));
    }
}

void Plotter::set_paint(const Paint& paint) {}

void Plotter::set_xform(M3f xform) {}

void Plotter::fill() {}

void Plotter::stroke() {}

void Plotter::write(std::string_view text) {}

void Plotter::reset() {
    NOTF_ASSERT(m_context.is_current());

    m_vertex_buffer->write().clear();
    m_index_buffer->write().clear();
    m_instance_buffer->write().clear();
    m_paint_buffer->write().clear();
    m_clipping_buffer->write().clear();

    m_paths.clear();
    m_drawcalls.clear();

    m_state = {};
}

void Plotter::render() const {}

void Plotter::_fill(const FillCall& call) {}

void Plotter::_stroke(const StrokeCall& call) {}

void Plotter::_write(const WriteCall& call) {}

// std::hash ======================================================================================================== //

/// std::hash specialization for Clipping.
template<>
struct ::std::hash<notf::Plotter::Clipping> {
    size_t operator()(const notf::Plotter::Clipping& clipping) const {
        return notf::hash(clipping.xform, clipping.size);
    }
};

/// std::hash specialization for FragmentPaint.
template<>
struct ::std::hash<notf::Plotter::FragmentPaint> {
    static_assert(sizeof(notf::Plotter::FragmentPaint) == 28 * sizeof(float));

    size_t operator()(const notf::Plotter::FragmentPaint& paint) const {
        const size_t* as_size_t = reinterpret_cast<const size_t*>(&paint);
        const uint32_t* as_uint = reinterpret_cast<const uint32_t*>(&paint);
        return notf::hash(as_size_t[0], as_size_t[1], as_size_t[2], as_uint[6]);
    }
};

/*

#include "notf/meta/log.hpp"

#include "notf/common/matrix4.hpp"
#include "notf/common/utf8.hpp"
#include "notf/common/variant.hpp"


void Plotter::stroke(valid_ptr<PathPtr> path, const Paint& paint, StrokeInfo info) {
    if (path->size == 0 || info.width <= 0.f) {
        return; // early out
    }

    // a line must be at least a pixel wide to be drawn, to emulate thinner lines lower the alpha
    info.width = max(1, info.width);

    // store the path
    info.path = std::move(path);

    // upload the paint and store the index
    info.paint_index = m_uniform_buffer->get_element_count();
    m_uniform_buffer->write().emplace_back(paint);

    // store the resulting draw call
    m_drawcalls.emplace_back(DrawCall{std::move(info)});
}

void Plotter::fill(valid_ptr<PathPtr> path, const Paint& paint, FillInfo info) {
    if (path->size == 0) {
        return; // early out
    }

    // store the path
    info.path = std::move(path);

    // upload the paint and store the index
    info.paint_index = m_uniform_buffer->get_element_count();
    m_uniform_buffer->write().emplace_back(paint);

    // store the resulting draw call
    m_drawcalls.emplace_back(DrawCall{std::move(info)});
}

void Plotter::write(const std::string& text, const Paint& paint, TextInfo info) {
    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

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
                VertexBuffer::vertex_t vertex;
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

    // store the path
    info.path = Path::_create_shared();
    info.path->offset = narrow_cast<uint>(index_offset);
    info.path->size = narrow_cast<int>(indices.size() - index_offset);

    // upload the paint and store the index
    info.paint_index = m_uniform_buffer->get_element_count();
    m_uniform_buffer->write().emplace_back(paint);
    // TODO: don't create a new paint entry in the uniform buffer if a similar one already exists

    // store the resulting draw call
    m_drawcalls.emplace_back(DrawCall{std::move(info)});
}


void Plotter::render() const {
    if (m_index_buffer->is_empty()) { return; }

    NOTF_ASSERT(m_context.is_current());
    m_context->vertex_object = m_vertex_object;
    m_context->program = m_program;

    m_context->uniform_slots[0].bind_block(m_program->get_uniform_block("PaintBlock"));

    m_vertex_buffer->upload();
    m_index_buffer->upload();
    m_uniform_buffer->upload();

    NOTF_CHECK_GL(glEnable(GL_CULL_FACE));
    NOTF_CHECK_GL(glCullFace(GL_BACK));
    NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, m_state.patch_vertices));
    NOTF_CHECK_GL(glEnable(GL_BLEND));
    NOTF_CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // screen size
    const Aabri& render_area = m_context->framebuffer.get_render_area();
    if (m_state.screen_size != render_area.get_size()) {
        m_state.screen_size = render_area.get_size();
        m_program->get_uniform("projection")
            .set(M4f::orthographic(0, m_state.screen_size.width(), 0, m_state.screen_size.height(), 0, 2));
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
        m_program->get_uniform("patch_type").set(to_number(PatchType::STROKE));
        m_state.patch_type = PatchType::STROKE;
    }

    // stroke width
    if (abs(m_state.stroke_width - stroke.width) > precision_high<float>()) {
        m_program->get_uniform("stroke_width").set(stroke.width);
        m_state.stroke_width = stroke.width;
    }

    // paint uniform block
    m_context->uniform_slots[0].bind_buffer(m_uniform_buffer, stroke.paint_index);

    NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(stroke.path->size), g_index_type,
                                 gl_buffer_offset(stroke.path->offset * sizeof(IndexBuffer::index_t))));
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
            m_program->get_uniform("patch_type").set(to_number(PatchType::CONVEX));
            m_state.patch_type = PatchType::CONVEX;
        }
    } else {
        if (m_state.patch_type != PatchType::CONCAVE) {
            m_program->get_uniform("patch_type").set(to_number(PatchType::CONCAVE));
            m_state.patch_type = PatchType::CONCAVE;
        }
    }

    // base vertex
    if (!shape.path->center.is_approx(m_state.vec2_aux1)) {
        // with a purely convex polygon, we can safely put the base vertex into the center of the polygon as it
        // will always be inside and it should never fall onto an existing vertex
        // this way, we can use antialiasing at the outer edge
        m_program->get_uniform("vec2_aux1").set(shape.path->center);
        m_state.vec2_aux1 = shape.path->center;
    }

    // paint uniform block
    m_context->uniform_slots[0].bind_buffer(m_uniform_buffer, shape.paint_index);

    if (shape.path->is_convex) {
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(shape.path->size), g_index_type,
                                     gl_buffer_offset(shape.path->offset * sizeof(IndexBuffer::index_t))));
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
                                     gl_buffer_offset(shape.path->offset * sizeof(IndexBuffer::index_t))));
        NOTF_CHECK_GL(glEnable(GL_CULL_FACE));

        NOTF_CHECK_GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)); // re-enable color
        NOTF_CHECK_GL(glStencilFunc(GL_NOTEQUAL, 0x00, 0xff));          // only write to pixels that are inside the
                                                                        // polygon
        NOTF_CHECK_GL(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO)); // reset the stencil buffer (is a lot faster than
                                                                        // clearing it at the start)

        // render colors here, same area as before if you don't want to clear the stencil buffer every time
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(shape.path->size), g_index_type,
                                     gl_buffer_offset(shape.path->offset * sizeof(IndexBuffer::index_t))));

        NOTF_CHECK_GL(glDisable(GL_STENCIL_TEST));
    }
}

void Plotter::_write(const WriteCall& call) {

    // patch vertices
    if (m_state.patch_vertices != 1) {
        m_state.patch_vertices = 1;
        NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, 1));
    }

    // patch type
    if (m_state.patch_type != PatchType::TEXT) {
        m_program->get_uniform("patch_type").set(to_number(PatchType::TEXT));
        m_state.patch_type = PatchType::TEXT;
    }

    { // atlas size
        const Size2i& atlas_size = TheGraphicsSystem()->get_font_manager().get_atlas_texture()->get_size();
        const V2f atlas_size_vec{atlas_size.width(), atlas_size.height()};
        if (!atlas_size_vec.is_approx(m_state.vec2_aux1)) {
            m_program->get_uniform("vec2_aux1").set(atlas_size_vec);
            m_state.vec2_aux1 = atlas_size_vec;
        }
    }

    // uniform blocks
    if (call.paint_index != m_state.paint_index) {
        m_context->uniform_slots[0].bind_buffer(m_paint_buffer, call.paint_index);
        m_state.paint_index = call.paint_index;
    }
    if (call.clip_index != m_state.clip_index) {
        m_context->uniform_slots[1].bind_buffer(m_clipping_buffer, call.clip_index);
        m_state.clip_index = call.clip_index;
    }

    { // draw
        NOTF_ASSERT(call.path_index < m_paths.size());
        const Path& path = m_paths[call.path_index];
        NOTF_CHECK_GL(glDrawElements(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                     gl_buffer_offset(path.offset * sizeof(IndexBuffer::index_t))));
    }
}



*/
