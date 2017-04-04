/*
 * The NoTF painting pipeline
 * ==========================
 *
 * When drawing Widgets with NoTF, the internal process goes through several stages.
 * The whole process is closely designed after the way nanovg (https://github.com/memononen/nanovg) handles its
 * rendering, but adds a level of caching intermediate data for faster redrawing of unchanged Widgets.
 *
 * Commands
 * ========
 * Most public functions accessible on a Painter do not immediately do what their documentation sais, but instead create
 * `commands` and stack them on the Painter's `command stack`.
 * These `commands` are not the Commands NoTF uses for user interaction, think of them like bytecode that the Painter
 * executes when it creates Paths.
 * That means that they are very fast to execute, even when called from Python.
 *
 * PainterPoints and PainterPaths
 * ==============================
 * After the Painter finished building its `command stack`, it is executed.
 * The final goal of the process is to create and store vertices in the Cell so that it can be rendered in OpenGL.
 * However, in order to reason about the user's intention, the execution of `commands` does not produce vertices
 * directly, but creates an intermediate representation: `PainterPoints` and `PainterPaths`.
 * While the user thinks of "paths" as shapes that can be built, combined and eventually filled or stroked, a
 * `PainterPath` object in the Painter implementation is simply a range of `PainterPoints`.
 *
 */

#include "graphics/cell/painter.hpp"

#include "common/line2.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/commands.hpp"
#include "graphics/render_context.hpp"

namespace { // anonymous
using namespace notf;

/** Length of a bezier control vector to draw a circle with radius 1. */
static const float KAPPAf = static_cast<float>(KAPPA);

float to_float(const Cell::Command command) { return static_cast<float>(to_number(command)); }

void transform_command_pos(const Xform2f& xform, std::vector<float>& commands, size_t index)
{
    assert(commands.size() >= index + 2);
    Vector2f& point = *reinterpret_cast<Vector2f*>(&commands[index]);
    point           = xform.transform(point);
}

/** Returns a Vector2f reference into the Command buffer without data allocation. */
Vector2f& as_vector2f(std::vector<float>& commands, size_t index)
{
    assert(index + 1 < commands.size());
    return *reinterpret_cast<Vector2f*>(&commands[index]);
}

} // namespace anonymous

namespace notf {

/**********************************************************************************************************************/

std::vector<detail::PainterState> Painter::s_states;

Painter::Painter(Cell& cell, RenderContext& context)
    : m_cell(cell)
    , m_context(context)
    , m_stylus(Vector2f::zero())
    , m_has_open_path(false)
{
    // states
    s_states.clear();
    s_states.emplace_back();

    // reset the Cell before adding any Commands
    m_cell.m_commands.clear();
}

size_t Painter::push_state()
{
    m_cell.m_commands.add_command(PushStateCommand());
    s_states.emplace_back(s_states.back());
    return s_states.size();
}

size_t Painter::pop_state()
{
    if (s_states.size() > 1) {
        m_cell.m_commands.add_command(PopStateCommand());
        s_states.pop_back();
    }
    return s_states.size();
}

void Painter::set_transform(const Xform2f xform)
{
    detail::PainterState& current_state = _get_current_state();
    current_state.xform                 = std::move(xform);
    m_cell.m_commands.add_command(SetXformCommand(current_state.xform));
}
void Painter::reset_transform()
{
    m_cell.m_commands.add_command(ResetXformCommand());
    _get_current_state().xform = Xform2f::identity();
}

void Painter::transform(const Xform2f& transform)
{
    m_cell.m_commands.add_command(TransformCommand(transform));
    _get_current_state().xform *= transform;
}

void Painter::translate(const Vector2f& delta)
{
    m_cell.m_commands.add_command(TranslationCommand(delta));
    _get_current_state().xform *= Xform2f::translation(delta);
}

void Painter::rotate(const float angle)
{
    m_cell.m_commands.add_command(RotationCommand(angle));
    _get_current_state().xform = Xform2f::rotation(angle) * _get_current_state().xform; // TODO: Transform2::premultiply
}

void Painter::set_scissor(const Aabrf& aabr)
{
    detail::PainterState& current_state = _get_current_state();

    current_state.scissor.xform = Xform2f::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend();
    m_cell.m_commands.add_command(SetScissorCommand(current_state.scissor));
}

void Painter::remove_scissor()
{
    m_cell.m_commands.add_command(ResetScissorCommand());
    _get_current_state().scissor = {Xform2f::identity(), {-1, -1}};
}

void Painter::set_blend_mode(const BlendMode mode)
{
    m_cell.m_commands.add_command(BlendModeCommand(mode));
    _get_current_state().blend_mode = mode;
}

void Painter::set_alpha(const float alpha)
{
    m_cell.m_commands.add_command(SetAlphaCommand(alpha));
    _get_current_state().alpha = alpha;
}

void Painter::set_miter_limit(const float limit)
{
    m_cell.m_commands.add_command(MiterLimitCommand(limit));
    _get_current_state().miter_limit = limit;
}

void Painter::set_line_cap(const LineCap cap)
{
    m_cell.m_commands.add_command(LineCapCommand(cap));
    _get_current_state().line_cap = cap;
}

void Painter::set_line_join(const LineJoin join)
{
    m_cell.m_commands.add_command(LineJoinCommand(join));
    _get_current_state().line_join = join;
}

void Painter::set_fill_paint(Paint paint)
{
    detail::PainterState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.fill_paint = std::move(paint);
    m_cell.m_commands.add_command(FillPaintCommand(current_state.fill_paint));
}

void Painter::set_fill_color(Color color)
{
    m_cell.m_commands.add_command(FillColorCommand(color));
    detail::PainterState& current_state = _get_current_state();
    current_state.fill_paint.set_color(std::move(color));
}

void Painter::set_stroke_paint(Paint paint)
{
    detail::PainterState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.stroke_paint = std::move(paint);
    m_cell.m_commands.add_command(StrokePaintCommand(current_state.stroke_paint));
}

void Painter::set_stroke_color(Color color)
{
    m_cell.m_commands.add_command(StrokeColorCommand(color));
    detail::PainterState& current_state = _get_current_state();
    current_state.stroke_paint.set_color(std::move(color));
}

void Painter::set_stroke_width(const float width)
{
    detail::PainterState& current_state = _get_current_state();
    current_state.stroke_width          = max(0.f, width);
    m_cell.m_commands.add_command(StrokeWidthCommand(current_state.stroke_width));
}

void Painter::begin_path()
{
    m_cell.m_commands.clear();
    m_cell.m_paths.clear();
    m_cell.m_points.clear();
    m_cell.m_vertices.clear();
    m_stylus = Vector2f::zero();
}

void Painter::close_path()
{
    m_cell.m_commands.add_command(CloseCommand());
    m_has_open_path = false;
}

void Painter::set_winding(const Winding winding)
{
    // TODO: SetWindingCommand has no effect when there's no Path yet. That's not a problem per se, but optimizable? Same goes for close
    m_cell.m_commands.add_command(SetWindingCommand(winding));
}

void Painter::move_to(const Vector2f pos)
{
    m_cell.m_commands.add_command(MoveCommand(std::move(pos)));
}

void Painter::line_to(const Vector2f pos)
{
    m_cell.m_commands.add_command(LineCommand(std::move(pos)));
    m_has_open_path = true;
}

void Painter::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    // convert the quad spline into a bezier
    m_cell.m_commands.add_command(BezierCommand(
        {m_stylus.x + (2.f / 3.f) * (cx - m_stylus.x),
         m_stylus.y + (2.f / 3.f) * (cy - m_stylus.y)},
        {tx + (2.f / 3.f) * (cx - tx),
         ty + (2.f / 3.f) * (cy - ty)},
        {tx, ty}));
    m_has_open_path = true;
}

void Painter::bezier_to(const Vector2f ctrl1, const Vector2f ctrl2, const Vector2f end)
{
    m_cell.m_commands.add_command(BezierCommand(std::move(ctrl1), std::move(ctrl2), std::move(end)));
    m_has_open_path = true;
}

void Painter::arc(const float cx, const float cy, const float r, const float a0, const float a1, const Winding dir)
{
    // clamp angles
    float da;
    if (dir == Winding::CLOCKWISE) {
        da = norm_angle(a1 - a0 - static_cast<float>(PI)) + static_cast<float>(PI);
    }
    else {
        da = norm_angle(a1 - a0 + static_cast<float>(PI)) - static_cast<float>(PI);
    }
    // split the arc into <= 90deg segments
    const float ndivs = max(1.f, min(ceilf(abs(da) / static_cast<float>(HALF_PI)), 5.f));
    const float hda   = (da / ndivs) / 2;
    const float kappa = abs(4.f / 3.f * (1.0f - cos(hda)) / sin(hda)) * (dir == Winding::CLOCKWISE ? 1 : -1);

    // create individual commands
    std::vector<float> commands(static_cast<size_t>(ceilf(ndivs)) * 7 + 3);
    size_t command_index = 0;
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    for (float i = 0; i <= ndivs; i++) {
        assert(command_index < commands.size());
        const float a    = a0 + da * (i / ndivs);
        const float dx   = cos(a);
        const float dy   = sin(a);
        const float x    = cx + dx * r;
        const float y    = cy + dy * r;
        const float tanx = -dy * r * kappa;
        const float tany = dx * r * kappa;
        if (command_index == 0) {
            commands[command_index++] = to_float(m_cell.m_commands.empty() ? Cell::Command::MOVE : Cell::Command::LINE);
            commands[command_index++] = x;
            commands[command_index++] = y;
        }
        else {
            commands[command_index++] = to_float(Cell::Command::BEZIER);
            commands[command_index++] = px + ptanx;
            commands[command_index++] = py + ptany;
            commands[command_index++] = x - tanx;
            commands[command_index++] = y - tany;
            commands[command_index++] = x;
            commands[command_index++] = y;
        }
        px    = x;
        py    = y;
        ptanx = tanx;
        ptany = tany;
    }
    _append_commands(std::move(commands));
}

void Painter::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    m_cell.m_commands.reserve(m_cell.m_commands.size() + commands.size());

    // commands operate in the context's current transformation space, but we need them in global space
    const Xform2f& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size();) {
        Cell::Command command = static_cast<Cell::Command>(commands[i]);
        switch (command) {

        case Cell::Command::MOVE:
        case Cell::Command::LINE: {
            m_stylus = as_vector2f(commands, i + 1);
            transform_command_pos(xform, commands, i + 1);
            i += 3;
        } break;

        case Cell::Command::BEZIER: {
            m_stylus = as_vector2f(commands, i + 5);
            transform_command_pos(xform, commands, i + 1);
            transform_command_pos(xform, commands, i + 3);
            transform_command_pos(xform, commands, i + 5);
            i += 7;
        } break;

        case Cell::Command::WINDING:
            i += 2;
            break;

        case Cell::Command::CLOSE:
            i += 1;
            break;

        default:
            assert(0);
        }
    }

    // finally, append the new commands to the existing ones
    std::move(commands.begin(), commands.end(), std::back_inserter(m_cell.m_commands));
}

void Painter::arc_to(const Vector2f& tangent, const Vector2f& end, const float radius)
{
    // handle degenerate cases
    const float distance_tolerance = m_context.get_distance_tolerance();
    if (radius < distance_tolerance
        || m_stylus.is_approx(tangent, distance_tolerance)
        || tangent.is_approx(end, distance_tolerance)
        || Line2::from_points(m_stylus, end).closest_point(tangent).magnitude_sq() < (distance_tolerance * distance_tolerance)) {
        return line_to(end);
    }

    // calculate tangential circle to lines (stylus -> tangent) and (tangent -> end)
    const Vector2f delta1 = (m_stylus - tangent).normalize();
    const Vector2f delta2 = (tangent - end).normalize();
    const float a         = acos(delta1.dot(delta2)) / 2;
    if (a == approx(0.)) {
        return line_to(end);
    }
    const float d = radius / tan(a);
    if (d > 10000.0f) { // I guess any large number would do
        return line_to(end);
    }

    // prepare the call to `arc` from the known arguments
    float cx, cy, a0, a1;
    Winding dir;
    if (delta1.cross(delta2) < 0) {
        cx  = tangent.x + delta1.x * d + delta1.y * radius;
        cy  = tangent.y + delta1.y * d + -delta1.x * radius;
        a0  = atan2(delta1.x, -delta1.y);
        a1  = atan2(-delta2.x, delta2.y);
        dir = Winding::CLOCKWISE;
    }
    else {
        cx  = tangent.x + delta1.x * d + -delta1.y * radius;
        cy  = tangent.y + delta1.y * d + delta1.x * radius;
        a0  = atan2(-delta1.x, delta1.y);
        a1  = atan2(delta2.x, -delta2.y);
        dir = Winding::COUNTERCLOCKWISE;
    }
    arc(cx, cy, radius, a0, a1, dir);
}

void Painter::add_rect(const float x, const float y, const float w, const float h)
{
    m_cell.m_commands.add_command(MoveCommand({x, y}));
    m_cell.m_commands.add_command(LineCommand({x, y + h}));
    m_cell.m_commands.add_command(LineCommand({x + w, y + h}));
    m_cell.m_commands.add_command(LineCommand({x + w, y}));
    close_path();
}

void Painter::add_rounded_rect(const float x, const float y, const float w, const float h,
                               const float rtl, const float rtr, const float rbr, const float rbl)
{
    if (rtl < 0.1f && rtr < 0.1f && rbr < 0.1f && rbl < 0.1f) {
        add_rect(std::move(x), std::move(y), std::move(w), std::move(h));
        return;
    }
    const float halfw = abs(w) * 0.5f;
    const float halfh = abs(h) * 0.5f;
    const float rxBL = min(rbl, halfw) * sign(w), ryBL = min(rbl, halfh) * sign(h);
    const float rxBR = min(rbr, halfw) * sign(w), ryBR = min(rbr, halfh) * sign(h);
    const float rxTR = min(rtr, halfw) * sign(w), ryTR = min(rtr, halfh) * sign(h);
    const float rxTL = min(rtl, halfw) * sign(w), ryTL = min(rtl, halfh) * sign(h);
    m_cell.m_commands.add_command(MoveCommand({x, y + ryTL}));
    m_cell.m_commands.add_command(LineCommand({x, y + h - ryBL}));
    m_cell.m_commands.add_command(BezierCommand({x, y + h - ryBL * (1 - KAPPAf)}, {x + rxBL * (1 - KAPPAf), y + h}, {x + rxBL, y + h}));
    m_cell.m_commands.add_command(LineCommand({x + w - rxBR, y + h}));
    m_cell.m_commands.add_command(BezierCommand({x + w - rxBR * (1 - KAPPAf), y + h}, {x + w, y + h - ryBR * (1 - KAPPAf)}, {x + w, y + h - ryBR}));
    m_cell.m_commands.add_command(LineCommand({x + w, y + ryTR}));
    m_cell.m_commands.add_command(BezierCommand({x + w, y + ryTR * (1 - KAPPAf)}, {x + w - rxTR * (1 - KAPPAf), y}, {x + w - rxTR, y}));
    m_cell.m_commands.add_command(LineCommand({x + rxTL, y}));
    m_cell.m_commands.add_command(BezierCommand({x + rxTL * (1 - KAPPAf), y}, {x, y + ryTL * (1 - KAPPAf)}, {x, y + ryTL}));
    close_path();
}

void Painter::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    m_cell.m_commands.add_command(MoveCommand({cx - rx, cy}));
    m_cell.m_commands.add_command(BezierCommand({cx - rx, cy + ry * KAPPAf}, {cx - rx * KAPPAf, cy + ry}, {cx, cy + ry}));
    m_cell.m_commands.add_command(BezierCommand({cx + rx * KAPPAf, cy + ry}, {cx + rx, cy + ry * KAPPAf}, {cx + rx, cy}));
    m_cell.m_commands.add_command(BezierCommand({cx + rx, cy - ry * KAPPAf}, {cx + rx * KAPPAf, cy - ry}, {cx, cy - ry}));
    m_cell.m_commands.add_command(BezierCommand({cx - rx * KAPPAf, cy - ry}, {cx - rx, cy - ry * KAPPAf}, {cx - rx, cy}));
    close_path();
}

void Painter::fill()
{
    const detail::PainterState& state = _get_current_state();
    Paint fill_paint                  = state.fill_paint;

    // apply global alpha
    fill_paint.inner_color.a *= state.alpha;
    fill_paint.outer_color.a *= state.alpha;

    _flatten_paths();
    _expand_fill(m_context.provides_geometric_aa());
    m_context.add_fill_call(fill_paint, m_cell);
}

void Painter::_expand_fill(const bool draw_antialiased)
{
    const float fringe = draw_antialiased ? m_context.get_fringe_width() : 0;
    assert(fringe >= 0);

    _calculate_joins(fringe, LineJoin::MITER, 2.4f);

    const float woff = fringe / 2;
    for (Cell::Path& path : m_cell.m_paths) {
        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        assert(last_point_offset < m_cell.m_points.size());

        // create the fill vertices
        path.fill_offset = m_cell.m_vertices.size();
        if (fringe > 0) {
            // create a loop
            for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Cell::Point& previous_point = m_cell.m_points[previous_offset];
                const Cell::Point& current_point  = m_cell.m_points[current_offset];

                // no bevel
                if (!(current_point.flags & Cell::Point::Flags::BEVEL) || current_point.flags & Cell::Point::Flags::LEFT) {
                    m_cell.m_vertices.emplace_back(current_point.pos + (current_point.dm * woff),
                                                   Vector2f(.5f, 1));
                }

                // beveling requires an extra vertex
                else {
                    m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * woff,
                                                            current_point.pos.y - previous_point.forward.x * woff),
                                                   Vector2f(.5f, 1));
                    m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * woff,
                                                            current_point.pos.y - current_point.forward.x * woff),
                                                   Vector2f(.5f, 1));
                }
            }
        }
        // no fringe = no antialiasing
        else {
            for (size_t point_offset = path.point_offset; point_offset <= last_point_offset; ++point_offset) {
                m_cell.m_vertices.emplace_back(m_cell.m_points[point_offset].pos,
                                               Vector2f(.5f, 1));
            }
        }
        path.fill_count = m_cell.m_vertices.size() - path.fill_offset;

        // create stroke vertices, if we draw this shape antialiased
        if (fringe > 0) {
            path.stroke_offset = m_cell.m_vertices.size();

            float left_w        = fringe + woff;
            float left_u        = 0;
            const float right_w = fringe - woff;
            const float right_u = 1;

            { // create only half a fringe for convex shapes so that the shape can be rendered without stenciling
                const bool is_convex = m_cell.m_paths.size() == 1 && m_cell.m_paths.front().is_convex;
                if (is_convex) {
                    left_w = woff; // this should generate the same vertex as fill inset above
                    left_u = 0.5f; // set outline fade at middle
                }
            }

            // create a loop
            for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Cell::Point& previous_point = m_cell.m_points[previous_offset];
                const Cell::Point& current_point  = m_cell.m_points[current_offset];

                if (current_point.flags & (Cell::Point::Flags::BEVEL | Cell::Point::Flags::INNERBEVEL)) {
                    _bevel_join(previous_point, current_point, left_w, right_w, left_u, right_u);
                }
                else {
                    m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.dm.x * left_w,
                                                            current_point.pos.y + current_point.dm.y * left_w),
                                                   Vector2f(left_u, 1));
                    m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.dm.x * right_w,
                                                            current_point.pos.y - current_point.dm.y * right_w),
                                                   Vector2f(right_u, 1));
                }
            }

            // copy the first two vertices from the beginning to form a cohesive loop
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[path.stroke_offset + 0].pos,
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[path.stroke_offset + 1].pos,
                                           Vector2f(right_u, 1));

            path.stroke_count = m_cell.m_vertices.size() - path.stroke_offset;
        }
    }
}

void Painter::_add_point(const Vector2f position, const Cell::Point::Flags flags)
{
    // if the Cell::Point is not significantly different, use the last one instead.
    if (!m_cell.m_points.empty()) {
        Cell::Point& last_point = m_cell.m_points.back();
        if (position.is_approx(last_point.pos, m_context.get_distance_tolerance())) {
            last_point.flags = static_cast<Cell::Point::Flags>(last_point.flags | flags);
            return;
        }
    }

    // otherwise create a new Cell::Point
    m_cell.m_points.emplace_back(Cell::Point{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});

    // and append it to the last path
    assert(!m_cell.m_paths.empty());
    m_cell.m_paths.back().point_count++;
}

void Painter::_tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                                const float x3, const float y3, const float x4, const float y4)
{
    static const int one = 1 << 10;

    // Power basis.
    const float ax = -x1 + 3 * x2 - 3 * x3 + x4;
    const float ay = -y1 + 3 * y2 - 3 * y3 + y4;
    const float bx = 3 * x1 - 6 * x2 + 3 * x3;
    const float by = 3 * y1 - 6 * y2 + 3 * y3;
    const float cx = -3 * x1 + 3 * x2;
    const float cy = -3 * y1 + 3 * y2;

    // Transform to forward difference basis (stepsize 1)
    float px   = x1;
    float py   = y1;
    float dx   = ax + bx + cx;
    float dy   = ay + by + cy;
    float ddx  = 6 * ax + 2 * bx;
    float ddy  = 6 * ay + 2 * by;
    float dddx = 6 * ax;
    float dddy = 6 * ay;

    int t           = 0;
    int dt          = one;
    const float tol = m_context.get_tesselation_tolerance() * 4.f;

    while (t < one) {

        // flatness measure (guessed)
        float d = ddx * ddx + ddy * ddy + dddx * dddx + dddy * dddy;

        // go to higher resolution if we're moving a lot, or overshooting the end.
        while ((d > tol && dt > 1) || (t + dt > one)) {

            // apply L to the curve. Increase curve resolution.
            dx   = .5f * dx - (1.f / 8.f) * ddx + (1.f / 16.f) * dddx;
            dy   = .5f * dy - (1.f / 8.f) * ddy + (1.f / 16.f) * dddy;
            ddx  = (1.f / 4.f) * ddx - (1.f / 8.f) * dddx;
            ddy  = (1.f / 4.f) * ddy - (1.f / 8.f) * dddy;
            dddx = (1.f / 8.f) * dddx;
            dddy = (1.f / 8.f) * dddy;

            // half the stepsize.
            dt >>= 1;

            d = ddx * ddx + ddy * ddy + dddx * dddx + dddy * dddy;
        }

        // go to lower resolution if we're really flat and we aren't going to overshoot the end.
        // tol/32 is just a guess for when we are too flat.
        while ((d > 0 && d < tol / 32.f && dt < one) && (t + 2 * dt <= one)) {

            // apply L^(-1) to the curve. Decrease curve resolution.
            dx   = 2 * dx + ddx;
            dy   = 2 * dy + ddy;
            ddx  = 4 * ddx + 4 * dddx;
            ddy  = 4 * ddy + 4 * dddy;
            dddx = 8 * dddx;
            dddy = 8 * dddy;

            // double the stepsize.
            dt <<= 1;

            d = ddx * ddx + ddy * ddy + dddx * dddx + dddy * dddy;
        }

        // forward differencing.
        px += dx;
        py += dy;
        dx += ddx;
        dy += ddy;
        ddx += dddx;
        ddy += dddy;

        _add_point({px, py}, (t > 0 ? Cell::Point::Flags::CORNER : Cell::Point::Flags::NONE));

        // advance along the curve.
        t += dt;
        assert(t <= one);
    }
}

void Painter::_flatten_paths()
{
    if (!m_cell.m_paths.empty()) {
        return; // This feels weird and I will change it in the rework discussed at the end of the file
    }

    // parse the command buffer
    for (size_t index = 0; index < m_cell.m_commands.size();) {
        switch (static_cast<Cell::Command>(m_cell.m_commands[index])) {

        case Cell::Command::MOVE:
            m_cell.m_paths.emplace_back(m_cell.m_points.size());
            _add_point(*reinterpret_cast<Vector2f*>(&m_cell.m_commands[index + 1]), Cell::Point::Flags::CORNER);
            index += 3;
            break;

        case Cell::Command::LINE:
            if (m_cell.m_paths.empty()) {
                m_cell.m_paths.emplace_back(m_cell.m_points.size());
            }
            _add_point(*reinterpret_cast<Vector2f*>(&m_cell.m_commands[index + 1]), Cell::Point::Flags::CORNER);
            index += 3;
            break;

        case Cell::Command::BEZIER:
            if (m_cell.m_points.empty()) {
                m_cell.m_points.emplace_back(Cell::Point{Vector2f::zero(), Vector2f::zero(), Vector2f::zero(), 0.f, Cell::Point::Flags::NONE});
            }
            if (m_cell.m_paths.empty()) {
                m_cell.m_paths.emplace_back(m_cell.m_points.size());
            }
            _tesselate_bezier(m_cell.m_points.back().pos.x, m_cell.m_points.back().pos.y,
                              m_cell.m_commands[index + 1], m_cell.m_commands[index + 2],
                              m_cell.m_commands[index + 3], m_cell.m_commands[index + 4],
                              m_cell.m_commands[index + 5], m_cell.m_commands[index + 6]);
            index += 7;
            break;

        case Cell::Command::CLOSE:
            if (!m_cell.m_paths.empty()) {
                m_cell.m_paths.back().is_closed = true;
            }
            ++index;
            break;

        case Cell::Command::WINDING:
            if (!m_cell.m_paths.empty()) {
                m_cell.m_paths.back().winding = static_cast<unsigned char>(m_cell.m_commands[index + 1]);
            }
            index += 2;
            break;

        default: // should never happen
            assert(0);
            ++index;
        }
    }

    m_cell.m_bounds = Aabrf::wrongest();
    for (size_t path_index = 0; path_index < m_cell.m_paths.size(); ++path_index) {
        Cell::Path& path = m_cell.m_paths[path_index];

        if (path.point_count >= 2) {
            // if the first and last Cell::Points are the same, remove the last one and mark the path as closed
            const Cell::Point& first = m_cell.m_points[path.point_offset];
            const Cell::Point& last  = m_cell.m_points[path.point_offset + path.point_count - 1];
            if (first.pos.is_approx(last.pos, m_context.get_distance_tolerance())) {
                --path.point_count;
                path.is_closed = true;
            }
        }

        if (path.point_count < 2) {
            // remove paths with just a single Cell::Point
            m_cell.m_paths.erase(iterator_at(m_cell.m_paths, path_index));
            --path_index;
            continue;
        }

        // enforce winding
        const float area = poly_area(m_cell.m_points, path.point_offset, path.point_count);
        if ((path.winding == 0 && area < 0)
            || (path.winding == 1 && area > 0)) {
            for (size_t i = path.point_offset, j = path.point_offset + path.point_count - 1; i < j; ++i, --j) {
                std::swap(m_cell.m_points[i], m_cell.m_points[j]);
            }
        }

        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        for (size_t next_offset = path.point_offset, current_offset = last_point_offset;
             next_offset <= last_point_offset;
             current_offset = next_offset++) {
            Cell::Point& current_point = m_cell.m_points[current_offset];
            Cell::Point& next_point    = m_cell.m_points[next_offset];

            // calculate segment delta
            current_point.forward = next_point.pos - current_point.pos;
            current_point.length  = current_point.forward.magnitude();
            if (current_point.length > 0) {
                current_point.forward /= current_point.length;
            }

            // update bounds
            m_cell.m_bounds._min.x = min(m_cell.m_bounds._min.x, current_point.pos.x);
            m_cell.m_bounds._min.y = min(m_cell.m_bounds._min.y, current_point.pos.y);
            m_cell.m_bounds._max.x = max(m_cell.m_bounds._max.x, current_point.pos.x);
            m_cell.m_bounds._max.y = max(m_cell.m_bounds._max.y, current_point.pos.y);
        }
    }
}

float Painter::poly_area(const std::vector<Cell::Point>& points, const size_t offset, const size_t count)
{
    float area           = 0;
    const Cell::Point& a = points[0];
    for (size_t index = offset + 2; index < offset + count; ++index) {
        const Cell::Point& b = points[index - 1];
        const Cell::Point& c = points[index];
        area += (c.pos.x - a.pos.x) * (b.pos.y - a.pos.y) - (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y);
    }
    return area / 2;
}

void Painter::stroke()
{
    m_cell.m_commands.add_command(StrokeCommand());
}

Time Painter::get_time() const
{
    return m_context.get_time();
}

const Vector2f& Painter::get_mouse_pos() const
{
    return m_context.get_mouse_pos();
}

void Painter::_calculate_joins(const float fringe, const LineJoin join, const float miter_limit)
{
    for (Cell::Path& path : m_cell.m_paths) {
        size_t left_turn_count = 0;

        // Calculate which joins needs extra vertices to append, and gather vertex count.
        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
             current_offset <= last_point_offset;
             previous_offset = current_offset++) {
            const Cell::Point& previous_point = m_cell.m_points[previous_offset];
            Cell::Point& current_point        = m_cell.m_points[current_offset];

            // clear all flags except the corner
            current_point.flags = (current_point.flags & Cell::Point::Flags::CORNER) ? Cell::Point::Flags::CORNER : Cell::Point::Flags::NONE;

            // keep track of left turns
            if (current_point.forward.cross(previous_point.forward) > 0) {
                ++left_turn_count;
                current_point.flags = static_cast<Cell::Point::Flags>(current_point.flags | Cell::Point::Flags::LEFT);
            }

            // calculate extrusions
            current_point.dm.x    = (previous_point.forward.y + current_point.forward.y) / 2.f;
            current_point.dm.y    = (previous_point.forward.x + current_point.forward.x) / -2.f;
            const float dm_mag_sq = current_point.dm.magnitude_sq();
            if (dm_mag_sq > 0.000001f) {
                float scale = 1.0f / dm_mag_sq;
                if (scale > 600.0f) {
                    scale = 600.0f;
                }
                current_point.dm.x *= scale;
                current_point.dm.y *= scale;
            }

            // Calculate if we should use bevel or miter for inner join.
            float limit = max(1.01f, min(previous_point.length, current_point.length) * (fringe > 0.0f ? 1.0f / fringe : 0.f));
            if ((dm_mag_sq * limit * limit) < 1.0f) {
                current_point.flags = static_cast<Cell::Point::Flags>(current_point.flags | Cell::Point::Flags::INNERBEVEL);
            }

            // Check to see if the corner needs to be beveled.
            if (current_point.flags & Cell::Point::Flags::CORNER) {
                if (join == LineJoin::BEVEL || join == LineJoin::ROUND || (dm_mag_sq * miter_limit * miter_limit) < 1.0f) {
                    current_point.flags = static_cast<Cell::Point::Flags>(current_point.flags | Cell::Point::Flags::BEVEL);
                }
            }
        }

        path.is_convex = (left_turn_count == path.point_count);
    }
}

void Painter::_bevel_join(const Cell::Point& previous_point, const Cell::Point& current_point, const float left_w, const float right_w,
                          const float left_u, const float right_u)
{
    if (current_point.flags & Cell::Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(current_point.flags & Cell::Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, left_w);

        m_cell.m_vertices.emplace_back(Vector2f(lx0, ly0),
                                       Vector2f(left_u, 1));
        m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                                current_point.pos.y + previous_point.forward.x * right_w),
                                       Vector2f(right_u, 1));

        if (current_point.flags & Cell::Point::Flags::BEVEL) {
            m_cell.m_vertices.emplace_back(Vector2f(lx0, ly0),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                                    current_point.pos.y + previous_point.forward.x * right_w),
                                           Vector2f(right_u, 1));

            m_cell.m_vertices.emplace_back(Vector2f(lx1, ly1),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                                    current_point.pos.y + current_point.forward.x * right_w),
                                           Vector2f(right_u, 1));
        }
        else {
            const float rx0 = current_point.pos.x - current_point.dm.x * right_w;
            const float ry0 = current_point.pos.y - current_point.dm.y * right_w;

            m_cell.m_vertices.emplace_back(current_point.pos,
                                           Vector2f(0.5f, 1));
            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                                    current_point.pos.y + previous_point.forward.x * right_w),
                                           Vector2f(right_u, 1));

            m_cell.m_vertices.emplace_back(Vector2f(rx0, ry0),
                                           Vector2f(right_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(rx0, ry0),
                                           Vector2f(right_u, 1));

            m_cell.m_vertices.emplace_back(current_point.pos,
                                           Vector2f(0.5f, 1));
            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                                    current_point.pos.y + current_point.forward.x * right_w),
                                           Vector2f(right_u, 1));
        }

        m_cell.m_vertices.emplace_back(Vector2f(lx1, ly1),
                                       Vector2f(left_u, 1));
        m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                                current_point.pos.y + current_point.forward.x * right_w),
                                       Vector2f(right_u, 1));
    }

    // right corner
    else {
        float rx0, ry0, rx1, ry1;
        std::tie(rx0, ry0, rx1, ry1) = choose_bevel(current_point.flags & Cell::Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, -right_w);

        m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                                current_point.pos.y - previous_point.forward.x * left_w),
                                       Vector2f(left_u, 1));
        m_cell.m_vertices.emplace_back(Vector2f(rx0, ry0),
                                       Vector2f(right_u, 1));

        if (current_point.flags & Cell::Point::Flags::BEVEL) {
            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                                    current_point.pos.y - previous_point.forward.x * left_w),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(rx0, ry0),
                                           Vector2f(right_u, 1));

            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                                    current_point.pos.y - current_point.forward.x * left_w),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(rx1, ry1),
                                           Vector2f(right_u, 1));
        }
        else {
            const float lx0 = current_point.pos.x + current_point.dm.x * left_w;
            const float ly0 = current_point.pos.y + current_point.dm.y * left_w;

            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                                    current_point.pos.y - previous_point.forward.x * left_w),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(current_point.pos,
                                           Vector2f(0.5f, 1));

            m_cell.m_vertices.emplace_back(Vector2f(lx0, ly0),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(Vector2f(lx0, ly0),
                                           Vector2f(left_u, 1));

            m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                                    current_point.pos.y - current_point.forward.x * left_w),
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(current_point.pos,
                                           Vector2f(0.5f, 1));
        }

        m_cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                                current_point.pos.y - current_point.forward.x * left_w),
                                       Vector2f(left_u, 1));
        m_cell.m_vertices.emplace_back(Vector2f(rx1, ry1),
                                       Vector2f(right_u, 1));
    }
}

std::tuple<float, float, float, float>
Painter::choose_bevel(bool is_beveling, const Cell::Point& prev_point, const Cell::Point& curr_point, const float stroke_width)
{
    float x0, y0, x1, y1;
    if (is_beveling) {
        x0 = curr_point.pos.x + prev_point.forward.y * stroke_width;
        y0 = curr_point.pos.y - prev_point.forward.x * stroke_width;
        x1 = curr_point.pos.x + curr_point.forward.y * stroke_width;
        y1 = curr_point.pos.y - curr_point.forward.x * stroke_width;
    }
    else {
        x0 = curr_point.pos.x + curr_point.dm.x * stroke_width;
        y0 = curr_point.pos.y + curr_point.dm.y * stroke_width;
        x1 = curr_point.pos.x + curr_point.dm.x * stroke_width;
        y1 = curr_point.pos.y + curr_point.dm.y * stroke_width;
    }
    return std::make_tuple(x0, y0, x1, y1);
}

// TODO: the Painter might as well optimize the Commands given before sending them off to the cell.
// For example, if you set a paint, set color a, set color b and fill, setting color a is irrelevant.
// Also, if you set a complete new paint, it might be better to split it up in several commands that only change
// what is really changed, so instead of having two set_paint commands where only the fill color differs you have one
// set_paint command and a set_fill_color command
// Superfluous path close commands

} // namespace notf
