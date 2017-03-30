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
#include "graphics/cell/painterpreter.hpp"
#include "graphics/render_context.hpp"
#include "graphics/vertex.hpp"
#include "utils/enum_to_number.hpp"

namespace { // anonymous
using namespace notf;

/** Returns a Vector2f reference into the Command buffer without data allocation. */
Vector2f& as_vector2f(std::vector<float>& commands, size_t index)
{
    assert(index + 1 < commands.size());
    return *reinterpret_cast<Vector2f*>(&commands[index]);
}

/** Extracts a position from the given command buffer and applies a given transformation to it in-place. */
void transform_command_pos(std::vector<float>& commands, size_t index, const Xform2f& xform)
{
    assert(commands.size() >= index + 2);
    Vector2f& vector = as_vector2f(commands, index);
    vector           = xform.transform(vector);
}

/** Length of a bezier control vector to draw a circle with radius 1. */
static const float KAPPAf = static_cast<float>(KAPPA);

static Painterpreter INTERPRETER;

} // namespace anonymous

namespace notf {

/**********************************************************************************************************************/

std::vector<Painter::State> Painter::s_states;

Painter::Painter(Cell& cell, const RenderContext& context)
    : m_cell(cell)
    , m_context(context)
    , m_stylus(Vector2f::zero())
{
    // states
    s_states.clear();
    s_states.emplace_back();

    // setup the Painterpreter
    const float pixel_ratio = m_context.get_pixel_ratio();
    assert(abs(pixel_ratio) > 0);
    INTERPRETER.reset();
    INTERPRETER.set_distance_tolerance(m_context.get_distance_tolerance());
    INTERPRETER.set_tesselation_tolerance(m_context.get_tesselation_tolerance());
    INTERPRETER.set_fringe_width(m_context.get_fringe_width());
}

size_t Painter::push_state()
{
    INTERPRETER.add_command(PushStateCommand());
    s_states.emplace_back(s_states.back());
    return s_states.size();
}

size_t Painter::pop_state()
{
    if (s_states.size() > 1) {
        INTERPRETER.add_command(PopStateCommand());
        s_states.pop_back();
    }
    return s_states.size();
}

void Painter::set_scissor(const Aabrf& aabr)
{
    State& current_state        = _get_current_state();
    current_state.scissor.xform = Xform2f::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend();
}

void Painter::set_fill_paint(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.fill = std::move(paint);
}

void Painter::set_fill_color(Color color)
{
    State& current_state = _get_current_state();
    current_state.fill.set_color(std::move(color));
}

void Painter::set_stroke_paint(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.stroke = std::move(paint);
}

void Painter::set_stroke_color(Color color)
{
    State& current_state = _get_current_state();
    current_state.stroke.set_color(std::move(color));
}

void Painter::begin_path()
{
    INTERPRETER.add_command(BeginCommand());
}

void Painter::close_path()
{
    INTERPRETER.add_command(CloseCommand());
}

void Painter::set_winding(const Winding winding)
{
    INTERPRETER.add_command(SetWindingCommand(winding));
}

void Painter::move_to(const Vector2f pos)
{
    INTERPRETER.add_command(MoveCommand(std::move(pos)));
}

void Painter::line_to(const Vector2f pos)
{
    INTERPRETER.add_command(LineCommand(std::move(pos)));
}

void Painter::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    // convert the quad spline into a bezier
    INTERPRETER.add_command(BezierCommand(
        {m_stylus.x + (2.f / 3.f) * (cx - m_stylus.x),
         m_stylus.y + (2.f / 3.f) * (cy - m_stylus.y)},
        {tx + (2.f / 3.f) * (cx - tx),
         ty + (2.f / 3.f) * (cy - ty)},
        {tx, ty}));
}

void Painter::bezier_to(const Vector2f ctrl1, const Vector2f ctrl2, const Vector2f end)
{
    INTERPRETER.add_command(BezierCommand(std::move(ctrl1), std::move(ctrl2), std::move(end)));
}

void Painter::arc(float cx, float cy, float r, float a0, float a1, Winding dir)
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
    const float kappa = abs(4.f / 3.f * (1.0f - cos(hda)) / sin(hda)) * (dir == Winding::CLOCKWISE ? 1 : -1);

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
            if (INTERPRETER.is_current_path_empty()) {
                INTERPRETER.add_command(MoveCommand({x, y}));
            }
            else {
                INTERPRETER.add_command(LineCommand({x, y}));
            }
        }
        else {
            INTERPRETER.add_command(BezierCommand({px + ptanx, py + ptany}, {x - tanx, y - tany}, {x, y}));
        }
        px    = x;
        py    = y;
        ptanx = tanx;
        ptany = tany;
    }
}

void Painter::arc_to(const Vector2f& tangent, const Vector2f& end, const float radius)
{
    // handle degenerate cases
    if (radius < INTERPRETER.get_distance_tolerance()
        || m_stylus.is_approx(tangent, INTERPRETER.get_distance_tolerance())
        || tangent.is_approx(end, INTERPRETER.get_distance_tolerance())
        || Line2::from_points(m_stylus, end).closest_point(tangent).magnitude_sq()
            < (INTERPRETER.get_distance_tolerance() * INTERPRETER.get_distance_tolerance())) {
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
    INTERPRETER.add_command(MoveCommand({x, y}));
    INTERPRETER.add_command(LineCommand({x, y + h}));
    INTERPRETER.add_command(LineCommand({x + w, y + h}));
    INTERPRETER.add_command(LineCommand({x + w, y}));
    INTERPRETER.add_command(CloseCommand());
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
    INTERPRETER.add_command(MoveCommand({x, y + ryTL}));
    INTERPRETER.add_command(LineCommand({x, y + h - ryBL}));
    INTERPRETER.add_command(BezierCommand({x, y + h - ryBL * (1 - KAPPAf)}, {x + rxBL * (1 - KAPPAf), y + h}, {x + rxBL, y + h}));
    INTERPRETER.add_command(LineCommand({x + w - rxBR, y + h}));
    INTERPRETER.add_command(BezierCommand({x + w - rxBR * (1 - KAPPAf), y + h}, {x + w, y + h - ryBR * (1 - KAPPAf)}, {x + w, y + h - ryBR}));
    INTERPRETER.add_command(LineCommand({x + w, y + ryTR}));
    INTERPRETER.add_command(BezierCommand({x + w, y + ryTR * (1 - KAPPAf)}, {x + w - rxTR * (1 - KAPPAf), y}, {x + w - rxTR, y}));
    INTERPRETER.add_command(LineCommand({x + rxTL, y}));
    INTERPRETER.add_command(BezierCommand({x + rxTL * (1 - KAPPAf), y}, {x, y + ryTL * (1 - KAPPAf)}, {x, y + ryTL}));
    INTERPRETER.add_command(CloseCommand());
}

void Painter::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    INTERPRETER.add_command(MoveCommand({cx - rx, cy}));
    INTERPRETER.add_command(BezierCommand({cx - rx, cy + ry * KAPPAf}, {cx - rx * KAPPAf, cy + ry}, {cx, cy + ry}));
    INTERPRETER.add_command(BezierCommand({cx + rx * KAPPAf, cy + ry}, {cx + rx, cy + ry * KAPPAf}, {cx + rx, cy}));
    INTERPRETER.add_command(BezierCommand({cx + rx, cy - ry * KAPPAf}, {cx + rx * KAPPAf, cy - ry}, {cx, cy - ry}));
    INTERPRETER.add_command(BezierCommand({cx - rx * KAPPAf, cy - ry}, {cx - rx, cy - ry * KAPPAf}, {cx - rx, cy}));
    INTERPRETER.add_command(CloseCommand());
}

void Painter::fill()
{
    INTERPRETER.add_command(FillCommand());
}

void Painter::stroke()
{
    INTERPRETER.add_command(StrokeCommand());
}

Time Painter::get_time() const
{
    return m_context.get_time();
}

const Vector2f& Painter::get_mouse_pos() const
{
    return m_context.get_mouse_pos();
}


} // namespace notf
