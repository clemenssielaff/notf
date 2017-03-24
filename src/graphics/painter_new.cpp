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

#include "graphics/painter_new.hpp"

#include "common/line2.hpp"
#include "graphics/render_context.hpp"
#include "utils/range.hpp"

namespace { // anonymous
using namespace notf;

/**********************************************************************************************************************/

/** Command identifyers, type must be of the same size as a float. */
enum class Command : uint32_t {
    NEXT_STATE,
    RESET,
    WINDING,
    CLOSE,
    MOVE,
    LINE,
    BEZIER,
    FILL,
    STROKE,
};

static_assert(sizeof(Command) == sizeof(float),
              "Floats on your system don't seem be to be 32 bits wide. "
              "Adjust the type of the underlying type of Painter::Command to fit your particular system.");

/** Transforms a Command to a float value that can be stored in the Command buffer. */
float to_float(const Command command) { return static_cast<float>(to_number(command)); }

/**********************************************************************************************************************/

struct PainterPoint {
    enum Flags : uint {
        NONE       = 0,
        CORNER     = 1u << 1,
        LEFT       = 1u << 2,
        BEVEL      = 1u << 3,
        INNERBEVEL = 1u << 4,
    };

    /** Position of the Point. */
    Vector2f pos;

    /** Direction to the next Point. */
    Vector2f forward;

    /** Something with miter...? */
    Vector2f dm;

    /** Distance to the next point forward. */
    float length;

    /** Additional information about this Point. */
    Flags flags;
};

/**********************************************************************************************************************/

struct PainterPath {

    PainterPath(std::vector<PainterPoint>& points)
        : range(make_range(points, points.size(), points.size()))
        , is_closed(false)
        , winding(Painter::Winding::COUNTERCLOCKWISE)
    {
    }

    /** Range of Points that make up this Path. */
    Range<std::vector<PainterPoint>::iterator> range;

    /** Whether this Path is closed or not.
     * Closed Paths will draw an additional line between their last and first Point.
     */
    bool is_closed;

    /** What direction the Path is wound. */
    Painter::Winding winding;
};

/**********************************************************************************************************************/

/** The Painter class makes use of a lot of stacks: Commands, Points and Paths.
 * Instead of reallocating new vectors for each Painter, they all share the same vectors.
 * This way, we should keep memory use and - dynamic allocation to a minimum.
 * The PainterData is currently represented by a single, static and global object `g_data` which is fine as long as
 * `Painter` is used in a single-threaded enivironment.
 * If we decide to multi-thread Painter, there should be one PainterData object per thread.
 */
struct PainterData {

    void clear()
    {
        paths.clear();
        points.clear();
        commands.clear();
    }

    /** Appends a new Point to the current Path. */
    void add_point(const Vector2f position, const PainterPoint::Flags flags)
    {
        assert(!paths.empty());

        // if the point is not significantly different from the last one, use that instead.
        if (!points.empty()) {
            PainterPoint& last_point = points.back();
            if (position.is_approx(last_point.pos, distance_tolerance)) {
                last_point.flags = static_cast<PainterPoint::Flags>(last_point.flags | flags);
                return;
            }
        }

        // otherwise append create a new point to the current path
        points.emplace_back(PainterPoint{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});
        paths.back().range.grow_one();
    }

    /** Creates a new, empty Path. */
    void add_path() { paths.emplace_back(points); }

public: // fields
    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<float> commands;

    /** Points making up the Painter Paths. */
    std::vector<PainterPoint> points;

    /** Intermediate representation of the Painter Paths. */
    std::vector<PainterPath> paths;

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float distance_tolerance;

    /** Tesselation density when creating rounded shapes. */
    float tesselation_tolerance;

    float fringe_width;
};

PainterData g_data;

/**********************************************************************************************************************/

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

/** Tesellates a Bezier curve.
 * Note that I am using the (experimental) improvement on the standard nanovg tesselation algorithm here,
 * as found at: https://github.com/memononen/nanovg/issues/328
 * If there seems to be an issue with the tesselation, revert back to the "official" implementation.
 */
void tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
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
    const float tol = g_data.tesselation_tolerance * 4.f;

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

        g_data.add_point({px, py}, (t > 0 ? PainterPoint::Flags::CORNER : PainterPoint::Flags::NONE));

        // advance along the curve.
        t += dt;
        assert(t <= one);
    }
}

/**********************************************************************************************************************/

/** Length of a bezier control vector to draw a circle with radius 1. */
static const float KAPPAf = static_cast<float>(KAPPA);

} // namespace anonymous

namespace notf {

/**********************************************************************************************************************/

Paint Paint::create_linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                    const Color start_color, const Color end_color)
{
    static const float large_number = 1e5;

    Vector2f delta  = end_pos - start_pos;
    const float mag = delta.magnitude();
    if (mag == approx(0., 0.0001)) {
        delta.x = 0;
        delta.y = 1;
    }
    else {
        delta.x /= mag;
        delta.y /= mag;
    }

    Paint paint;
    paint.xform[0][0]   = delta.y;
    paint.xform[0][1]   = -delta.x;
    paint.xform[1][0]   = delta.x;
    paint.xform[1][1]   = delta.y;
    paint.xform[2][0]   = start_pos.x - (delta.x * large_number);
    paint.xform[2][1]   = start_pos.y - (delta.y * large_number);
    paint.radius        = 0.0f;
    paint.feather       = max(1.0f, mag);
    paint.extent.width  = large_number;
    paint.extent.height = large_number + (mag / 2);
    paint.inner_color   = std::move(start_color);
    paint.outer_color   = std::move(end_color);
    return paint;
}

Paint Paint::create_radial_gradient(const Vector2f& center,
                                    const float inner_radius, const float outer_radius,
                                    const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Xform2f::translation(center);
    paint.radius        = (inner_radius + outer_radius) * 0.5f;
    paint.feather       = max(1.f, outer_radius - inner_radius);
    paint.extent.width  = paint.radius;
    paint.extent.height = paint.radius;
    paint.inner_color   = std::move(inner_color);
    paint.outer_color   = std::move(outer_color);
    return paint;
}

Paint Paint::create_box_gradient(const Vector2f& center, const Size2f& extend,
                                 const float radius, const float feather,
                                 const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Xform2f::translation({center.x + extend.width / 2, center.y + extend.height / 2});
    paint.radius        = radius;
    paint.feather       = max(1.f, feather);
    paint.extent.width  = extend.width / 2;
    paint.extent.height = extend.height / 2;
    paint.inner_color   = std::move(inner_color);
    paint.outer_color   = std::move(outer_color);
    return paint;
}

Paint Paint::create_texture_pattern(const Vector2f& top_left, const Size2f& extend,
                                    std::shared_ptr<Texture2> texture,
                                    const float angle, const float alpha)
{
    Paint paint;
    paint.xform         = Xform2f::rotation(angle);
    paint.xform[2][0]   = top_left.x;
    paint.xform[2][1]   = top_left.y;
    paint.extent.width  = extend.width;
    paint.extent.height = extend.height;
    paint.texture       = texture;
    paint.inner_color   = Color(1, 1, 1, alpha);
    paint.outer_color   = Color(1, 1, 1, alpha);
    return paint;
}

/**********************************************************************************************************************/

std::vector<Painter::State> Painter::s_states;
std::vector<size_t> Painter::s_state_succession;

Painter::Painter(Cell& cell, const RenderContext& context)
    : m_cell(cell)
    , m_stylus(Vector2f::zero())
    , m_state_index(0)
{
    // states
    s_states.clear();
    s_states.emplace_back();
    s_state_succession.clear();
    s_state_succession.emplace_back(0);

    // data
    const float pixel_ratio = context.get_pixel_ratio();
    assert(abs(pixel_ratio) > 0);
    g_data.clear();
    g_data.distance_tolerance    = 0.01f / pixel_ratio;
    g_data.tesselation_tolerance = 0.25f / pixel_ratio;
    g_data.fringe_width          = 1.f / pixel_ratio;
}

size_t Painter::push_state()
{
    size_t stack_height  = 2;
    State& current_state = _get_current_state();
    for (size_t prev_index = current_state.previous_state; prev_index != State::INVALID_INDEX; ++stack_height) {
        prev_index = s_states[prev_index].previous_state;
    }
    s_states.emplace_back(current_state);
    s_states.back().previous_state = s_state_succession.back();
    m_state_index = s_states.size() - 1;
    s_state_succession.push_back(m_state_index);
    _append_commands({to_float(Command::NEXT_STATE)});
    return stack_height;
}

size_t Painter::pop_state()
{
    size_t stack_height = 1;
    if (s_state_succession.size() == 1) {
        return stack_height;
    }
    m_state_index = _get_current_state().previous_state;
    s_state_succession.push_back(m_state_index);
    _append_commands({to_float(Command::NEXT_STATE)});
    for (size_t prev_index = _get_current_state().previous_state; prev_index != State::INVALID_INDEX; ++stack_height) {
        prev_index = s_states[prev_index].previous_state;
    }
    return stack_height;
}

void Painter::set_scissor(const Aabrf& aabr)
{
    State& current_state        = _get_current_state();
    current_state.scissor.xform = Xform2f::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend();
}

void Painter::begin_path()
{
    _append_commands({to_float(Command::RESET)});
}

void Painter::close_path()
{
    _append_commands({to_float(Command::CLOSE)});
}

void Painter::set_winding(const Winding winding)
{
    _append_commands({to_float(Command::WINDING), static_cast<float>(to_number(winding))});
}

void Painter::move_to(const float x, const float y)
{
    _append_commands({to_float(Command::MOVE), x, y});
}

void Painter::line_to(const float x, const float y)
{
    _append_commands({to_float(Command::LINE), x, y});
}

void Painter::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    _append_commands({
        to_float(Command::BEZIER),
        m_stylus.x + (2.f / 3.f) * (cx - m_stylus.x),
        m_stylus.y + (2.f / 3.f) * (cy - m_stylus.y),
        tx + (2.f / 3.f) * (cx - tx),
        ty + (2.f / 3.f) * (cy - ty),
        tx,
        ty,
    });
}

void Painter::bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty)
{
    _append_commands({to_float(Command::BEZIER), c1x, c1y, c2x, c2y, tx, ty});
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
            commands[command_index++] = to_float(g_data.commands.empty() ? Command::MOVE : Command::LINE);
            commands[command_index++] = x;
            commands[command_index++] = y;
        }
        else {
            commands[command_index++] = to_float(Command::BEZIER);
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

void Painter::arc_to(const Vector2f& tangent, const Vector2f& end, const float radius)
{
    // handle degenerate cases
    if (radius < g_data.distance_tolerance
        || m_stylus.is_approx(tangent, g_data.distance_tolerance)
        || tangent.is_approx(end, g_data.distance_tolerance)
        || Line2::from_points(m_stylus, end).closest_point(tangent).magnitude_sq() < (g_data.distance_tolerance * g_data.distance_tolerance)) {
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
    _append_commands({
        to_float(Command::MOVE), x, y,
        to_float(Command::LINE), x, y + h,
        to_float(Command::LINE), x + w, y + h,
        to_float(Command::LINE), x + w, y,
        to_float(Command::CLOSE),
    });
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
    _append_commands({to_float(Command::MOVE), x, y + ryTL,
                      to_float(Command::LINE), x, y + h - ryBL,
                      to_float(Command::BEZIER), x, y + h - ryBL * (1 - KAPPAf), x + rxBL * (1 - KAPPAf), y + h, x + rxBL, y + h,
                      to_float(Command::LINE), x + w - rxBR, y + h,
                      to_float(Command::BEZIER), x + w - rxBR * (1 - KAPPAf), y + h, x + w, y + h - ryBR * (1 - KAPPAf), x + w, y + h - ryBR,
                      to_float(Command::LINE), x + w, y + ryTR,
                      to_float(Command::BEZIER), x + w, y + ryTR * (1 - KAPPAf), x + w - rxTR * (1 - KAPPAf), y, x + w - rxTR, y,
                      to_float(Command::LINE), x + rxTL, y,
                      to_float(Command::BEZIER), x + rxTL * (1 - KAPPAf), y, x, y + ryTL * (1 - KAPPAf), x, y + ryTL,
                      to_float(Command::CLOSE)});
}

void Painter::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    _append_commands({to_float(Command::MOVE), cx - rx, cy,
                      to_float(Command::BEZIER), cx - rx, cy + ry * KAPPAf, cx - rx * KAPPAf, cy + ry, cx, cy + ry,
                      to_float(Command::BEZIER), cx + rx * KAPPAf, cy + ry, cx + rx, cy + ry * KAPPAf, cx + rx, cy,
                      to_float(Command::BEZIER), cx + rx, cy - ry * KAPPAf, cx + rx * KAPPAf, cy - ry, cx, cy - ry,
                      to_float(Command::BEZIER), cx - rx * KAPPAf, cy - ry, cx - rx, cy - ry * KAPPAf, cx - rx, cy,
                      to_float(Command::CLOSE)});
}

void Painter::fill()
{
    _append_commands({to_float(Command::FILL)});
}

void Painter::stroke()
{
    _append_commands({to_float(Command::STROKE)});
}

void Painter::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    g_data.commands.reserve(g_data.commands.size() + commands.size());

    // commands operate in the context's current transformation space, but we need them in global space
    const Xform2f& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size(); ++i) {
        Command command = static_cast<Command>(commands[i]);
        switch (command) {

        case Command::MOVE:
        case Command::LINE: {
            m_stylus = as_vector2f(commands, i + 1);
            transform_command_pos(commands, i + 1, xform);
            ;
            i += 2;
        } break;

        case Command::BEZIER: {
            m_stylus = as_vector2f(commands, i + 5);
            transform_command_pos(commands, i + 1, xform);
            transform_command_pos(commands, i + 3, xform);
            transform_command_pos(commands, i + 5, xform);
            i += 6;
        } break;

        case Command::WINDING:
            i += 1;
            break;

        case Command::NEXT_STATE:
        case Command::RESET:
        case Command::CLOSE:
        case Command::FILL:
        case Command::STROKE:
            break;

        default:
            assert(0); // unexpected enum
        }
    }

    // finally, append the new commands to the existing ones
    std::move(commands.begin(), commands.end(), std::back_inserter(g_data.commands));
}

// TODO: when this draws, revisit this function and see if you find a way to guarantee that there is always a path and point without if-statements all over the place
void Painter::_execute()
{
    assert(!s_state_succession.empty());
    m_state_index = s_state_succession[0];

    // parse the command buffer
    for (size_t index = 0; index < g_data.commands.size(); ++index) {
        switch (static_cast<Command>(g_data.commands[index])) {

        case Command::MOVE:
            g_data.add_path();
            g_data.add_point(as_vector2f(g_data.commands, index + 1), PainterPoint::Flags::CORNER);
            index += 2;
            break;

        case Command::LINE:
            if (g_data.paths.empty()) {
                g_data.add_path();
            }
            g_data.add_point(as_vector2f(g_data.commands, index + 1), PainterPoint::Flags::CORNER);
            index += 2;
            break;

        case Command::BEZIER:
            if (g_data.paths.empty()) {
                g_data.add_path();
            }
            if (g_data.points.empty()) {
                g_data.add_point(Vector2f::zero(), PainterPoint::Flags::CORNER);
            }
            tesselate_bezier(g_data.points.back().pos.x, g_data.points.back().pos.y,
                             g_data.commands[index + 1], g_data.commands[index + 2],
                             g_data.commands[index + 3], g_data.commands[index + 4],
                             g_data.commands[index + 5], g_data.commands[index + 6]);
            index += 6;
            break;

        case Command::CLOSE:
            if (!g_data.paths.empty()) {
                g_data.paths.back().is_closed = true;
            }
            break;

        case Command::WINDING:
            if (!g_data.paths.empty()) {
                g_data.paths.back().winding = static_cast<Winding>(g_data.commands[index + 1]);
            }
            index += 1;
            break;

        case Command::RESET:
            g_data.paths.clear();
            g_data.points.clear();
            m_stylus = Vector2f::zero();
            break;

        case Command::NEXT_STATE:
            // aaargh! m_state_index must be an index into the s_state_succession, not into s_states!
            // CONTINUE HERE
            break;

        case Command::FILL:
        case Command::STROKE:
            break;

        default: // unknown enum
            assert(0);
        }
    }
}

} // namespace notf
