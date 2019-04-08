#include "notf/graphic/plotter/plotter.hpp"

#include "notf/meta/log.hpp"

#include "notf/common/filesystem.hpp"
#include "notf/common/geo/bezier.hpp"
#include "notf/common/geo/matrix4.hpp"
#include "notf/common/geo/polyline.hpp"
#include "notf/common/utf8.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/plotter/design.hpp"
#include "notf/graphic/shader_program.hpp"
#include "notf/graphic/text/font_manager.hpp"
#include "notf/graphic/texture.hpp"
#include "notf/graphic/uniform_buffer.hpp"

NOTF_USING_NAMESPACE;

namespace {

using UsageHint = notf::detail::AnyOpenGLBuffer::UsageHint;
using Vertex = Plotter::Vertices::vertex_t;

static constexpr GLenum g_index_type = to_gl_type(Plotter::Indices::index_t{});

// vertex =========================================================================================================== //

/// Sets the position of a Vertex.
/// @param vertex   Vertex to modify.
/// @param pos      Position in global coordinates.
void set_pos(Vertex& vertex, V2f pos) { std::get<0>(vertex) = std::move(pos); }

/// Sets the control point on the left of the Vertex.
/// @param vertex   Vertex to modify.
/// @param pos      Position in global coordinates.
void set_left_ctrl(Vertex& vertex, V2f pos) { std::get<1>(vertex) = std::move(pos); }

/// Sets the control point on the right of the Vertex.
/// @param vertex   Vertex to modify.
/// @param pos      Position in global coordinates.
void set_right_ctrl(Vertex& vertex, V2f pos) { std::get<2>(vertex) = std::move(pos); }

/// Sets the control point on the left of the Vertex from a spline.
/// We might have to modify the given ctrl point so we can be certain that is stores the outgoing tangent
/// of the vertex. For that, we get the tangent either from the ctrl point itself (if the distance to the
/// vertex > 0) or by calculating the tangent from the spline at the vertex. The control point is then moved
/// onto the tangent, one unit further away from the vertex than originally. If we encounter a ctrl point in
/// the shader that is of unit length, we know that in reality it was positioned on the vertex itself and
/// still get the vertex tangent.
/// @param vertex   Vertex to modify.
/// @param bezier   Bezier segment on the right of the vertex position.
void set_left_ctrl(Vertex& vertex, const CubicBezier2f bezier) {
    V2f delta = bezier.get_vertex(2) - bezier.get_vertex(3);
    const float mag_sq = delta.get_magnitude_sq();
    if (is_zero(mag_sq, precision_high<float>())) {
        const SquareBezier2f derivate = bezier.get_derivate();
        set_left_ctrl(vertex, -derivate.interpolate(0).normalize());
    } else {
        set_left_ctrl(vertex, delta *= 1 + (1 / sqrt(mag_sq)));
    }
}

/// Sets the second control point from a spline.
/// See `set_ctrl1` on why this method is needed.
/// @param vertex   Vertex to modify.
/// @param bezier   Bezier segment on the right of the vertex position.
void set_right_ctrl(Vertex& vertex, const CubicBezier2f bezier) {
    V2f delta = bezier.get_vertex(1) - bezier.get_vertex(0);
    const float mag_sq = delta.get_magnitude_sq();
    if (is_zero(mag_sq, precision_high<float>())) {
        const SquareBezier2f derivate = bezier.get_derivate();
        set_right_ctrl(vertex, derivate.interpolate(0).normalize());
    } else {
        set_right_ctrl(vertex, delta *= 1 + (1 / sqrt(mag_sq)));
    }
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

Plotter::FragmentPaint::FragmentPaint(const Paint& paint, const Path2Ptr& stencil, const float stroke_width,
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
    //    if (!stencil.is_empty()) {
    //        clip_rotation[0] = stencil.get_xform()[0][0];
    //        clip_rotation[1] = stencil.get_xform()[0][1];
    //        clip_rotation[2] = stencil.get_xform()[1][0];
    //        clip_rotation[3] = stencil.get_xform()[1][1];
    //        clip_translation[0] = stencil.get_xform()[2][0];
    //        clip_translation[1] = stencil.get_xform()[2][1];
    //        clip_size[0] = static_cast<float>(stencil.get_size().width() / 2);
    //        clip_size[1] = static_cast<float>(stencil.get_size().height() / 2);
    //    }

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
        m_program->get_uniform("font_texture").set(TheGraphicsSystem::get_environment().font_atlas_texture_slot);
    }

    { // vertex object
        m_vertex_buffer = Vertices::create("PlotterVertexBuffer", UsageHint::STREAM_DRAW);
        m_index_buffer = Indices::create("PlotterIndexBuffer", UsageHint::STREAM_DRAW);
        m_xform_buffer
            = XformBuffer::create("PlotterInstanceBuffer", UsageHint::STREAM_DRAW, /*is_per_instance= */ true);
        m_vertex_object = VertexObject::create(m_context, "PlotterVertexObject");
        m_vertex_object->bind(m_xform_buffer, 3);
        m_vertex_object->bind(m_vertex_buffer, 0, 1, 2);
        m_vertex_object->bind(m_index_buffer);
    }

    { // uniform buffers
        m_paint_buffer = UniformBuffer<FragmentPaint>::create("PlotterPaintBuffer", UsageHint::STREAM_DRAW);
    }
}

Plotter::~Plotter() {
    NOTF_ASSERT(m_context.is_current()); // make sure the context is current when the Plotter is deletec
}

void Plotter::start_parsing() {
    NOTF_ASSERT(m_context.is_current());

    // clear server-side buffers
    m_vertex_buffer->write().clear();
    m_index_buffer->write().clear();
    m_xform_buffer->write().clear();
    m_paint_buffer->write().clear();
    // TODO: instead of clearing all plotter buffers each frame, anticipate paths
    //       In order to do that, keep a set of all paths around hat that have been drawn the last frame and if they are
    //       drawn again the next time, increase a counter. After a certain number of frames that drew a path, move the
    //       path into the front of the buffer and don't delete them during frames. Then, after a path hasn't been drawn
    //       for another number of frames, remove the path again. Ideally, we would be able to stack paths that are
    //       used more often earlier in the buffer than those that regularly move in and out of the buffer.

    // reset the plotter state
    m_paths.clear();
    m_path_lookup.clear();
    m_clips.clear();
    m_drawcalls.clear();
    m_states.clear();

    // TODO: mutable paths
    //       Immutable paths are great for shapes and icons etc. But there are two other kind of paths that would be
    //       nice to have in order to get maximum performance. Maybe they should be their own classes, maybe they should
    //       be some sort of variant (managed via a shared_ptr like the current Path2), we'll see.
    //       * The first type would be a mutable path, with a fixed number of vertices. Like an audio waveform or that
    //         example graph from the nanovg application. It would be wasteful to allocate a completely new vector each
    //         frame, but with the current Path2 we cannot modify the existing vertices one created. This type would
    //         re-use the client-side memory but still be threadsafe when read from the render thread. A block of server
    //         memory reserved for these kinds of paths would be updated not once but many times, each time with a very
    //         small update only. This can happen in parallel because we know the location of each path.
    //       * The second type is a straight-up mutable path, throwaway paths. Is there anything we can do then just to
    //         create new Path2Ptrs each time?
}

void Plotter::parse(const PlotterDesign& design, const M3f& xform, const Aabrf& clip) {
    // reset the state stack for each design
    m_states.clear();
    m_states.emplace_back();

    // adopt the Design's auxiliary information
    _get_state().xform = xform;
    _get_state().clip = clip;

    // parse the commands
    for (const PlotterDesign::Command& command : design.get_buffer()) {
        std::visit(
            overloaded{[&](const PlotterDesign::ResetState&) { _get_state() = {}; },
                       [&](const PlotterDesign::PushState&) { m_states.emplace_back(m_states.back()); },
                       [&](const PlotterDesign::PopState&) { _pop_state(); },
                       [&](const PlotterDesign::SetXform& cmd) { _get_state().xform = cmd.data->transformation; },
                       [&](const PlotterDesign::SetPaint& cmd) { _get_state().paint = cmd.data->paint; },
                       [&](const PlotterDesign::SetPath& cmd) { _get_state().path = cmd.data->path; },
                       [&](const PlotterDesign::SetClip& cmd) { _get_state().clip = cmd.data->clip; },
                       [&](const PlotterDesign::SetFont& cmd) { _get_state().font = cmd.data->font; },
                       [&](const PlotterDesign::SetAlpha& cmd) { _get_state().alpha = cmd.alpha; },
                       [&](const PlotterDesign::SetStrokeWidth& cmd) { _get_state().stroke_width = cmd.stroke_width; },
                       [&](const PlotterDesign::SetBlendMode& cmd) { _get_state().blend_mode = cmd.mode; },
                       [&](const PlotterDesign::SetLineCap& cmd) { _get_state().line_cap = cmd.cap; },
                       [&](const PlotterDesign::SetLineJoin& cmd) { _get_state().joint_style = cmd.join; },
                       [&](const PlotterDesign::Fill&) { _store_fill_call(); },
                       [&](const PlotterDesign::Stroke&) { _store_stroke_call(); },
                       [&](const PlotterDesign::Write& cmd) { _store_write_call(cmd.data->text); }},
            command);
    }
}

void Plotter::finish_parsing() {
    // early return if nothing was stored
    if (m_index_buffer->is_empty()) { return; }

    // set up the graphics context
    NOTF_ASSERT(m_context.is_current());
    m_context->vertex_object = m_vertex_object;
    m_context->program = m_program;
    m_context->uniform_slots[0].bind_block(m_program->get_uniform_block("PaintBlock"));
    m_context->uniform_slots[0].bind_buffer(m_paint_buffer);
    m_context->cull_face = CullFace::BACK;
    m_context->blend_mode = BlendMode(BlendMode::SOURCE_OVER2, BlendMode::SOURCE_OVER);
    m_server_state.paint_index = 0;

    // upload the buffers
    m_vertex_buffer->upload();
    m_index_buffer->upload();
    m_xform_buffer->upload();
    m_paint_buffer->upload();

    // screen size
    const Aabri& render_area = m_context->framebuffer.get_render_area();
    if (m_server_state.screen_size != render_area.get_size()) {
        m_server_state.screen_size = render_area.get_size();
        m_program->get_uniform("projection")
            .set(M4f::orthographic( //
                0, m_server_state.screen_size.width(), 0, m_server_state.screen_size.height(), 0, 2));
    }

    // perform the render calls
    for (const DrawCall& drawcall : m_drawcalls) {
        std::visit(overloaded{
                       [&](const _FillCall& call) { _render_fill(call); },
                       [&](const _StrokeCall& call) { _render_stroke(call); },
                       [&](const _WriteCall& call) { _render_text(call); },
                   },
                   drawcall);
    }
}

uint Plotter::_store_path(const Path2Ptr& p_path) {
    // return the index of an existing path
    if (auto itr = m_path_lookup.find(p_path); itr != m_path_lookup.end()) { return itr->second; }

    std::vector<Vertex>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

    // TODO: I had the idea of storing an offset into the Path2 itself, this way we were able to use instancing
    // more effectively (also see the xform buffer on the plotter, that is currently unused). But this idea
    // kinda fell off the wagon and is now left in limbo. For now, we use the transformation of the state to
    // create a tarnsformed copy of the path, which is shitty but works.
    const M3f& xform = _get_state().xform;
    NOTF_ASSERT(!p_path->get_subpaths().empty());
    Path2Ptr path = Path2::create(transform_by(p_path->get_subpaths().front().m_path, xform));

    // create the new path
    Path new_path;
    new_path.vertex_offset = narrow_cast<int>(vertices.size());
    new_path.index_offset = narrow_cast<uint>(indices.size());
    new_path.center = path->get_center();
    new_path.is_convex = path->is_convex();

    // store new vertices in the vertex- and index buffer
    for (const auto& subpath : path->get_subpaths()) {
        if (subpath.segment_count == 0) { continue; }

        { // create indices
            const size_t expected_size = indices.size() + (subpath.segment_count * 4) + (subpath.is_closed ? 0 : 2);
            indices.reserve(expected_size);

            const uint wrap_index = subpath.segment_count + (subpath.is_closed ? 0 : 1);
            for (uint i = 0, index = 0; i < subpath.segment_count; ++i) {
                // start cap / joint
                indices.emplace_back(index);
                indices.emplace_back(index);

                // segment
                indices.emplace_back(index);
                index = ++index % wrap_index;
                indices.emplace_back(index);
            }

            if (!subpath.is_closed) {
                // end cap
                indices.emplace_back(indices.back());
                indices.emplace_back(indices.back());
            }

            new_path.size = narrow_cast<int>(indices.size() - new_path.index_offset);
            NOTF_ASSERT(indices.size() == expected_size);
        }

        { // create vertices
            const size_t expected_size = vertices.size() + subpath.segment_count + (subpath.is_closed ? 0 : 1);
            vertices.reserve(expected_size);



            { // first vertex
                CubicBezier2f right_segment = subpath.get_segment(0);
                Vertex vertex;
                set_pos(vertex, right_segment.get_vertex(0));
                if (subpath.is_closed) {
                    set_left_ctrl(vertex, subpath.get_segment(subpath.segment_count - 1));
                } else {
                    set_left_ctrl(vertex, V2f::zero());
                }
                set_right_ctrl(vertex, std::move(right_segment));
                vertices.emplace_back(std::move(vertex));
            }

            // middle vertices
            for (size_t i = 0; i < subpath.segment_count - 1; ++i) {
                CubicBezier2f right_segment = subpath.get_segment(i + 1);
                Vertex vertex;
                set_pos(vertex, right_segment.get_vertex(0));
                set_left_ctrl(vertex, subpath.get_segment(i));
                set_right_ctrl(vertex, std::move(right_segment));
                vertices.emplace_back(std::move(vertex));
            }

            // last vertex
            if (!subpath.is_closed) {
                CubicBezier2f left_segment = subpath.get_segment(subpath.segment_count - 1);
                Vertex vertex;
                set_pos(vertex, left_segment.get_vertex(3));
                set_left_ctrl(vertex, std::move(left_segment));
                set_right_ctrl(vertex, V2f::zero());
                vertices.emplace_back(std::move(vertex));
            }

            NOTF_ASSERT(vertices.size() == expected_size);
        }
    }

    // store the path and keep the `shared_ptr` around for easy lookup in the future
    const uint index = narrow_cast<uint>(m_paths.size());
    m_path_lookup.emplace(path, index);
    m_paths.emplace_back(std::move(new_path));
    return index;
}

void Plotter::_store_call_base(_CallBase& call) {
    const PainterState& state = _get_state();
    call.alpha = clamp(state.alpha, 0, 1);
    call.blend_mode = state.blend_mode;

    { // store xform
        auto& xform_buffer = m_xform_buffer->write();
        if (xform_buffer.empty() || state.xform != std::get<0>(xform_buffer.back())) {
            xform_buffer.emplace_back(state.xform);
        }
        NOTF_ASSERT(!xform_buffer.empty());
        call.xform = narrow_cast<uint>(xform_buffer.size() - 1);
    }

    { // store paint
        auto& paint_buffer = m_paint_buffer->write();
        if (paint_buffer.empty() || true) { // || state.paint != paint_buffer.back()){
                                            // TODO squash similar paints or even index them like paths?
            Paint paint = state.paint;
            paint.inner_color.a *= state.alpha;
            paint.outer_color.a *= state.alpha;
            paint_buffer.emplace_back(state.paint);
        }
        NOTF_ASSERT(!paint_buffer.empty());
        call.paint = narrow_cast<uint>(paint_buffer.size() - 1);
    }

    { // store clip
        if (m_clips.empty() || state.clip != m_clips.back()) { m_clips.emplace_back(state.clip); }
        NOTF_ASSERT(!m_clips.empty());
        call.clip = narrow_cast<uint>(m_clips.size() - 1);
    }
}

void Plotter::_store_fill_call() {
    // early out, if the call would have no visible effect
    const PainterState& state = _get_state();
    if (state.path->is_empty()                                               // no path
        || is_zero(state.alpha, precision_low<float>())                      // transparent
        || is_zero(state.xform.get_determinant(), precision_low<float>())) { // xforms maps to zero area
        return;
    }

    // store call
    _FillCall call;
    _store_call_base(call);
    call.path = _store_path(state.path);
    m_drawcalls.emplace_back(std::move(call));
}

void Plotter::_store_stroke_call() {
    // early out, if the call would have no visible effect
    const PainterState& state = _get_state();
    if (!state.path || state.path->is_empty()                                // no path
        || is_zero(state.stroke_width, precision_low<float>())               // zero width
        || is_zero(state.alpha, precision_low<float>())                      // transparent
        || is_zero(state.xform.get_determinant(), precision_low<float>())) { // xforms maps to zero area
        return;
    }

    _StrokeCall call;
    _store_call_base(call);
    call.path = _store_path(state.path);
    call.cap = state.line_cap;
    call.join = state.joint_style;
    call.width = abs(state.stroke_width * state.xform.get_scale_factor());
    m_drawcalls.emplace_back(std::move(call));
}

void Plotter::_store_write_call(std::string text) {
    // early out, if the call would have no visible effect
    const PainterState& state = _get_state();
    if (text.empty()                                                         // no text
        || is_zero(state.alpha, precision_low<float>())                      // transparent
        || is_zero(state.xform.get_determinant(), precision_low<float>())) { // xforms maps to zero area
        return;
    }
    if (!state.font) {
        NOTF_LOG_WARN("Cannot render a text without a font"); // TODO Plotter default font?
        return;
    }

    std::vector<Vertex>& vertices = m_vertex_buffer->write();
    std::vector<GLuint>& indices = m_index_buffer->write();

    // create the new path
    Path new_path;
    new_path.vertex_offset = narrow_cast<int>(vertices.size());
    new_path.index_offset = narrow_cast<uint>(indices.size());

    { // vertices
        const Utf8 utf8_text(text);

        // make sure that text is always rendered on the pixel grid, not between pixels
        const V2f position = V2f::zero() * state.xform;
        Glyph::coord_t x = static_cast<Glyph::coord_t>(roundf(position.x()));
        Glyph::coord_t y = static_cast<Glyph::coord_t>(roundf(position.y()));
        // TODO Glyphs cannot be rotated, sheared or otherwise transformed, only translated
        //      In order to change this, I suspect that we should signed-distance font rendering? Or maybe have two
        //      different kinds of glyph rendering?

        for (const auto character : utf8_text) {
            const Glyph& glyph = state.font->get_glyph(static_cast<codepoint_t>(character));

            // skip glyphs without pixels
            if (!glyph.rect.width || !glyph.rect.height) { continue; }

            // quad which renders the glyph
            const Aabrf quad((x + glyph.left), (y - glyph.rect.height + glyph.top), //
                             glyph.rect.width, glyph.rect.height);

            // uv coordinates of the glyph - must be divided by the size of the font texture!
            const V2f uv = V2f{glyph.rect.x, glyph.rect.y};

            // create the vertex
            Vertex vertex;
            set_pos(vertex, uv);
            set_left_ctrl(vertex, quad.get_bottom_left());
            set_right_ctrl(vertex, quad.get_top_right());
            vertices.emplace_back(std::move(vertex));

            // advance to the next character position
            x += glyph.advance_x;
            y += glyph.advance_y;
        }
    }

    // indices
    indices.reserve(indices.size() + (vertices.size() - static_cast<uint>(new_path.vertex_offset)));
    for (GLuint i = static_cast<uint>(new_path.vertex_offset), end = narrow_cast<GLuint>(vertices.size()); i < end;
         ++i) {
        indices.emplace_back(i);
    }
    new_path.size = narrow_cast<int>(indices.size() - new_path.index_offset);

    // store the path (is interpreted as glyph positions when rendered)
    const uint path_index = narrow_cast<uint>(m_paths.size());
    m_paths.emplace_back(std::move(new_path));

    // store call
    _WriteCall call;
    _store_call_base(call);
    call.path = path_index;
    m_drawcalls.emplace_back(std::move(call));
}

void Plotter::_render_fill(const _FillCall& call) {}

void Plotter::_render_stroke(const _StrokeCall& stroke) {
    // patch vertices
    if (m_server_state.patch_vertices != 2) {
        m_server_state.patch_vertices = 2;
        NOTF_CHECK_GL(glPatchParameteri(GL_PATCH_VERTICES, 2));
    }

    // patch type
    if (m_server_state.patch_type != PatchType::STROKE) {
        m_program->get_uniform("patch_type").set(to_number(PatchType::STROKE));
        m_server_state.patch_type = PatchType::STROKE;
    }

    // stroke width
    if (abs(m_server_state.stroke_width - stroke.width) > precision_high<float>()) {
        m_program->get_uniform("stroke_width").set(stroke.width);
        m_server_state.stroke_width = stroke.width;
    }

    // cap style
    if (m_server_state.cap_style != stroke.cap) {
        m_program->get_uniform("cap_style").set(static_cast<int>(to_number(stroke.cap)));
        m_server_state.cap_style = stroke.cap;
    }

    // joint style
    if (m_server_state.joint_style != stroke.join) {
        m_program->get_uniform("joint_style").set(static_cast<int>(to_number(stroke.join)));
        m_server_state.joint_style = stroke.join;
    }

    // paint uniform block
    // m_context->uniform_slots[0].bind_buffer(m_paint_buffer, stroke.paint); // TODO: this breaks

    // draw it
    NOTF_ASSERT(stroke.path < m_paths.size());
    const Path& path = m_paths[stroke.path];
    NOTF_CHECK_GL(glDrawElementsBaseVertex(GL_PATCHES, static_cast<GLsizei>(path.size), g_index_type,
                                           gl_buffer_offset(path.index_offset * sizeof(Indices::index_t)),
                                           path.vertex_offset));
    // TODO use glDrawElementsIndirect with an indirect command buffer?
}

void Plotter::_render_text(const _WriteCall& call) {}

// std::hash ======================================================================================================== //

/// std::hash specialization for FragmentPaint.
template<>
struct std::hash<notf::Plotter::FragmentPaint> {
    static_assert(sizeof(notf::Plotter::FragmentPaint) == 28 * sizeof(float));

    size_t operator()(const notf::Plotter::FragmentPaint& paint) const {
        const size_t* as_size_t = reinterpret_cast<const size_t*>(&paint);
        const uint32_t* as_uint = reinterpret_cast<const uint32_t*>(&paint);
        return notf::hash(as_size_t[0], as_size_t[1], as_size_t[2], as_uint[6]);
    }
};

/*
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
