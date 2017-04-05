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
#include "graphics/cell/cell.hpp"
#include "graphics/cell/commands.hpp"
#include "graphics/render_context.hpp"

namespace { // anonymous
using namespace notf;

/** Length of a bezier control vector to draw a circle with radius 1. */
static const float KAPPAf = static_cast<float>(KAPPA);

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
    m_cell.m_commands.add_command(BeginCommand());
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
        assert(dir == Winding::COUNTERCLOCKWISE);
        da = norm_angle(a1 - a0 + static_cast<float>(PI)) - static_cast<float>(PI);
    }
    // split the arc into <= 90deg segments
    const float ndivs = max(1.f, min(ceilf(abs(da) / static_cast<float>(HALF_PI)), 5.f));
    const float hda   = (da / ndivs) / 2;
    const float kappa = abs(4.f / 3.f * (1.f - cos(hda)) / sin(hda)) * (dir == Winding::CLOCKWISE ? 1 : -1);

    // create individual commands
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    for (float i = 0; i <= ndivs; i++) {
        const float a    = a0 + da * (i / ndivs);
        const float dx   = cos(a);
        const float dy   = sin(a);
        const float x    = cx + dx * r;
        const float y    = cy + dy * r;
        const float tanx = -dy * r * kappa;
        const float tany = dx * r * kappa;
        if (static_cast<int>(i) == 0) {
            if (!m_has_open_path) {
                m_cell.m_commands.add_command(MoveCommand({x, y}));
            }
            else {
                m_cell.m_commands.add_command(LineCommand({x, y}));
            }
        }
        else {
            m_cell.m_commands.add_command(BezierCommand({px + ptanx, py + ptany}, {x - tanx, y - tany}, {x, y}));
        }
        px    = x;
        py    = y;
        ptanx = tanx;
        ptany = tany;
    }
    m_has_open_path = true;
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
    m_cell.m_commands.add_command(FillCommand());
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

// TODO: the Painter might as well optimize the Commands given before sending them off to the cell.
// For example, if you set a paint, set color a, set color b and fill, setting color a is irrelevant.
// Also, if you set a complete new paint, it might be better to split it up in several commands that only change
// what is really changed, so instead of having two set_paint commands where only the fill color differs you have one
// set_paint command and a set_fill_color command
// Superfluous path close commands

} // namespace notf