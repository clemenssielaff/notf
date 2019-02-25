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

#include "notf/graphic/plotter/plotter.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/filesystem.hpp"
#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/matrix4.hpp"
#include "notf/common/geo/polyline.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/plotter/design.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/uniform_buffer.hpp"

NOTF_USING_NAMESPACE;

namespace {

using UsageHint = notf::detail::AnyOpenGLBuffer::UsageHint;

static constexpr GLenum g_index_type = to_gl_type(Plotter::IndexBuffer::index_t{});

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

// path ============================================================================================================= //

//void Plotter::Path::Vertex::set_left_ctrl(const CubicBezier2f::Segment& spline)
//{
//    V2f delta = spline.ctrl2 - spline.end;
//    const float mag_sq = delta.get_magnitude_sq();
//    if (is_zero(mag_sq, precision_high<float>())) {
//        set_left_ctrl(-(spline.get_tangent(1).normalize()));
//    } else {
//        set_left_ctrl(delta *= 1 + (1 / sqrt(mag_sq)));
//    }
//}

//void Plotter::Path::Vertex::set_right_ctrl(const CubicBezier2f::Segment& spline) {
//    V2f delta = spline.ctrl1 - spline.start;
//    const float mag_sq = delta.get_magnitude_sq();
//    if (is_zero(mag_sq, precision_high<float>())) {
//        set_left_ctrl(spline.get_tangent(0).normalize());
//    } else {
//        set_left_ctrl(delta *= 1 + (1 / sqrt(mag_sq)));
//    }
//}

// make sure that the Vertex class does not add to the memory requirement
static_assert(sizeof(Plotter::Path::Vertex) == sizeof(float) * 6);

// fragment paint =================================================================================================== //

Plotter::FragmentPaint::FragmentPaint(const Paint& paint, const Path2& stencil, const float stroke_width,
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

    // stencil
    if (!stencil.is_empty()) {
        clip_rotation[0] = stencil.get_xform()[0][0];
        clip_rotation[1] = stencil.get_xform()[0][1];
        clip_rotation[2] = stencil.get_xform()[1][0];
        clip_rotation[3] = stencil.get_xform()[1][1];
        clip_translation[0] = stencil.get_xform()[2][0];
        clip_translation[1] = stencil.get_xform()[2][1];
        clip_size[0] = static_cast<float>(stencil.get_size().width() / 2);
        clip_size[1] = static_cast<float>(stencil.get_size().height() / 2);
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
}

void Plotter::start_parsing() {
    NOTF_ASSERT(m_context.is_current());

    m_vertex_buffer->write().clear();
    m_index_buffer->write().clear();
    m_instance_buffer->write().clear();
    m_paint_buffer->write().clear();

    m_paths.clear();
    m_drawcalls.clear();

    m_states.clear();
    m_server_state = {};
}

void Plotter::parse(const PlotterDesign& design) {
    { // TODO adopt the Widget's auxiliary information
        m_states.clear();
        m_states.emplace_back();

        //        m_state.xform = widget->get_xform<AnyWidget::Space::WINDOW>();
        //        _get_current_state().stencil = widget->get_clipping_rect();
    }

    { // parse the commands
        const std::vector<PlotterDesign::Command>& buffer = design.get_buffer();
        for (const PlotterDesign::Command& command : buffer) {
            std::visit(
                overloaded{
                    [&](const PlotterDesign::ResetState&) { m_states = {}; },
                    [&](const PlotterDesign::PushState&) { /*TODO*/ }, [&](const PlotterDesign::PopState&) { /*TODO*/ },
                    [&](const PlotterDesign::SetXform& cmd) { _get_state().xform = cmd.data->transformation; },
                    [&](const PlotterDesign::SetPaint& cmd) { _get_state().paint = cmd.data->paint; },
                    [&](const PlotterDesign::SetPath& cmd) { _get_state().path = cmd.data->path; },
                    [&](const PlotterDesign::SetStencil& cmd) { _get_state().stencil = cmd.data->path; },
                    [&](const PlotterDesign::SetFont& cmd) { _get_state().font = cmd.data->font; },
                    [&](const PlotterDesign::SetAlpha& cmd) { _get_state().alpha = cmd.alpha; },
                    [&](const PlotterDesign::SetStrokeWidth& cmd) { _get_state().stroke_width = cmd.stroke_width; },
                    [&](const PlotterDesign::SetBlendMode& cmd) { _get_state().blend_mode = cmd.mode; },
                    [&](const PlotterDesign::SetLineCap& cmd) { _get_state().line_cap = cmd.cap; },
                    [&](const PlotterDesign::SetLineJoin& cmd) { _get_state().line_join = cmd.join; },
                    [&](const PlotterDesign::Fill&) { _store_fill(); },
                    [&](const PlotterDesign::Stroke&) { _store_stroke(); },
                    [&](const PlotterDesign::Write& cmd) { _store_write(cmd.data->text); }},
                command);
        }
    }
}

void Plotter::finish_parsing() {
    if (m_index_buffer->is_empty()) { return; }

    NOTF_ASSERT(m_context.is_current());
    m_context->vertex_object = m_vertex_object;
    m_context->program = m_program;

    m_context->uniform_slots[0].bind_block(m_program->get_uniform_block("PaintBlock"));

    m_vertex_buffer->upload();
    m_index_buffer->upload();
    m_instance_buffer->upload();
    m_paint_buffer->upload();

    NOTF_CHECK_GL(glEnable(GL_CULL_FACE));
    NOTF_CHECK_GL(glCullFace(GL_BACK));
    NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, m_server_state.patch_vertices));
    NOTF_CHECK_GL(glEnable(GL_BLEND));
    NOTF_CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // screen size
    const Aabri& render_area = m_context->framebuffer.get_render_area();
    if (m_server_state.screen_size != render_area.get_size()) {
        m_server_state.screen_size = render_area.get_size();
        m_program->get_uniform("projection")
            .set(M4f::orthographic( //
                0, m_server_state.screen_size.width(), 0, m_server_state.screen_size.height(), 0, 2));
    }

    // perform the render calls
    for (const _DrawCall& drawcall : m_drawcalls) {
        std::visit(overloaded{
                       [&](const _FillCall& call) { _render_fill(call); },
                       [&](const _StrokeCall& call) { _render_stroke(call); },
                       [&](const _WriteCall& call) { _render_text(call); },
                   },
                   drawcall);
    }
}

void Plotter::_store_fill() {}

void Plotter::_store_stroke() {
    NOTF_ASSERT(!_get_state().path.is_empty());

    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

//    // path
//    _Path path;
//    path.offset = narrow_cast<uint>(indices.size());
//    path.center = polygon.get_center();
//    path.is_convex = polygon.is_convex();
//    path.is_closed = true;

//    { // update indices
//        indices.reserve(indices.size() + (polygon.get_size() * 2));

//        const GLuint first_index = narrow_cast<GLuint>(vertices.size());
//        GLuint next_index = first_index;

//        for (size_t i = 0; i < polygon.get_size() - 1; ++i) {
//            // first -> (n-1) segment
//            indices.emplace_back(next_index++);
//            indices.emplace_back(next_index);
//        }

//        // last segment
//        indices.emplace_back(next_index);
//        indices.emplace_back(first_index);
//    }
//    path.size = narrow_cast<int>(indices.size() - path.offset);

//    // vertices
//    vertices.reserve(vertices.size() + polygon.get_size());
//    for (const V2f& point : polygon.get_vertices()) {
//        VertexBuffer::vertex_t vertex;
//        set_pos(vertex, point);
//        set_first_ctrl(vertex, V2f::zero());
//        set_second_ctrl(vertex, V2f::zero());
//        vertices.emplace_back(std::move(vertex));
//    }
}

void Plotter::_store_write(std::string text) {}

void Plotter::_render_fill(const _FillCall& call) {}

void Plotter::_render_stroke(const _StrokeCall& call) {}

void Plotter::_render_text(const _WriteCall& call) {}

// void Plotter::set_shape(const CubicBezier2f& spline) {
//    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
//    std::vector<GLuint>& indices = m_index_buffer->write();

//    // path
//    _Path path;
//    path.offset = narrow_cast<uint>(indices.size());
//    path.center = V2f::zero(); // TODO: extract more Plotter::Path information from bezier splines
//    path.is_convex = false;    //
//    path.is_closed = spline.segments.front().start.is_approx(spline.segments.back().end);

//    { // update indices
//        indices.reserve(indices.size() + (spline.segments.size() * 4) + 2);

//        GLuint index = narrow_cast<GLuint>(vertices.size());

//        // start cap
//        indices.emplace_back(index);
//        indices.emplace_back(index);

//        for (size_t i = 0; i < spline.segments.size(); ++i) {
//            // segment
//            indices.emplace_back(index);
//            indices.emplace_back(++index);

//            // joint / end cap
//            indices.emplace_back(index);
//            indices.emplace_back(index);
//        }
//    }
//    path.size = narrow_cast<int>(indices.size() - path.offset);

//    { // vertices
//        vertices.reserve(vertices.size() + spline.segments.size() + 1);

//        { // first vertex
//            const CubicBezier2f::Segment& first_segment = spline.segments.front();
//            VertexBuffer::vertex_t vertex;
//            set_pos(vertex, first_segment.start);
//            set_first_ctrl(vertex, V2f::zero());
//            set_modified_second_ctrl(vertex, first_segment);
//            vertices.emplace_back(std::move(vertex));
//        }

//        // middle vertices
//        for (size_t i = 0; i < spline.segments.size() - 1; ++i) {
//            const CubicBezier2f::Segment& left_segment = spline.segments[i];
//            const CubicBezier2f::Segment& right_segment = spline.segments[i + 1];
//            VertexBuffer::vertex_t vertex;
//            set_pos(vertex, left_segment.end);
//            set_modified_first_ctrl(vertex, left_segment);
//            set_modified_second_ctrl(vertex, right_segment);
//            vertices.emplace_back(std::move(vertex));
//        }

//        { // last vertex
//            const CubicBezier2f::Segment& last_segment = spline.segments.back();
//            VertexBuffer::vertex_t vertex;
//            set_pos(vertex, last_segment.end);
//            set_modified_first_ctrl(vertex, last_segment);
//            set_second_ctrl(vertex, V2f::zero());
//            vertices.emplace_back(std::move(vertex));
//        }
//    }
//}

// void Plotter::set_shape(const Polygonf& polygon) {

//    std::vector<VertexBuffer::vertex_t>& vertices = m_vertex_buffer->write();
//    std::vector<GLuint>& indices = m_index_buffer->write();

//    // path
//    _Path path;
//    path.offset = narrow_cast<uint>(indices.size());
//    path.center = polygon.get_center();
//    path.is_convex = polygon.is_convex();
//    path.is_closed = true;

//    { // update indices
//        indices.reserve(indices.size() + (polygon.get_size() * 2));

//        const GLuint first_index = narrow_cast<GLuint>(vertices.size());
//        GLuint next_index = first_index;

//        for (size_t i = 0; i < polygon.get_size() - 1; ++i) {
//            // first -> (n-1) segment
//            indices.emplace_back(next_index++);
//            indices.emplace_back(next_index);
//        }

//        // last segment
//        indices.emplace_back(next_index);
//        indices.emplace_back(first_index);
//    }
//    path.size = narrow_cast<int>(indices.size() - path.offset);

//    // vertices
//    vertices.reserve(vertices.size() + polygon.get_size());
//    for (const V2f& point : polygon.get_vertices()) {
//        VertexBuffer::vertex_t vertex;
//        set_pos(vertex, point);
//        set_first_ctrl(vertex, V2f::zero());
//        set_second_ctrl(vertex, V2f::zero());
//        vertices.emplace_back(std::move(vertex));
//    }
//}

// std::hash ======================================================================================================== //

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

//    void Painterpreter::_fill() {
//    const State& state = _get_current_state();
//    //    if (!state.path || is_approx(state.alpha, 0)) {
//    //        return; // early out
//    //    }

//    // get the fill paint
//    Paint paint = state.fill_paint;
//    paint.inner_color.a *= state.alpha;
//    paint.outer_color.a *= state.alpha;

//    // plot the shape
//    //    Plotter::FillInfo fill_info;
//    //    m_plotter->fill(state.path, paint, std::move(fill_info));
//}

// void Painterpreter::_stroke() {
//    const State& state = _get_current_state();
//    //    if (!state.path || state.stroke_width <= 0 || is_approx(state.alpha, 0)) {
//    //        return; // early out
//    //    }

//    // get the stroke paint
//    Paint paint = state.stroke_paint;
//    paint.inner_color.a *= state.alpha;
//    paint.outer_color.a *= state.alpha;

//    // create a sane stroke width
//    float stroke_width;
//    {
//        const float scale = state.xform.get_scale_factor();
//        stroke_width = max(state.stroke_width * scale, 0);
//        if (stroke_width < 1) {
//            // if the stroke width is less than pixel size, use alpha to emulate coverage.
//            const float alpha = clamp(stroke_width, 0.0f, 1.0f);
//            const float alpha_squared = alpha * alpha; // since coverage is area, scale by alpha*alpha
//            paint.inner_color.a *= alpha_squared;
//            paint.outer_color.a *= alpha_squared;
//            stroke_width = 1;
//        }
//    }

//    // plot the stroke
//    //    Plotter::StrokeInfo stroke_info;
//    //    stroke_info.width = stroke_width;
//    //    m_plotter->stroke(state.path, paint, std::move(stroke_info));
//}

// void Painterpreter::_write(const std::string& text) {
//    const State& state = _get_current_state();
//    if (!state.font || is_approx(state.alpha, 0)) {
//        return; // early out
//    }

//    // get the text paint
//    Paint paint = state.stroke_paint;
//    paint.inner_color.a *= state.alpha;
//    paint.outer_color.a *= state.alpha;

//    // plot the text
//    //    Plotter::TextInfo text_info;
//    //    text_info.font = state.font;
//    //    text_info.translation = transform_by(V2f::zero(), state.xform);
//    //    m_plotter->write(text, paint, std::move(text_info));
//}
