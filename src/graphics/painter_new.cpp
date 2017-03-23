#include "graphics/painter_new.hpp"

#include "common/line2.hpp"
#include "graphics/render_context.hpp"

namespace { // anonymous
using namespace notf;

/**********************************************************************************************************************/

struct Point {
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

/** Extracts a position from the given command buffer and applies a given transformation to it in-place. */
void transform_command_pos(std::vector<float>& commands, size_t index, const Xform2f& xform)
{
    assert(commands.size() >= index + 2);
    Vector2f& vector = *reinterpret_cast<Vector2f*>(&commands[index]);
    vector           = xform.transform(vector);
}

/**********************************************************************************************************************/

/** Intermediate representation of the drawn shapes. */
std::vector<Point> m_points;

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

std::vector<float> Painter::s_commands;

float Painter::_to_float(const Command command) { return static_cast<float>(to_number(command)); }

Painter::Painter(Cell& cell, const RenderContext& context)
    : m_cell(cell)
    , m_states({State()})
    , m_stylus()
    , m_tesselation_tolerance(0.25f / context.get_pixel_ratio())
    , m_distance_tolerance(0.01f / context.get_pixel_ratio())
    , m_fringe_width(1.f / context.get_pixel_ratio())
{
    //    m_cell.reset();
    begin_path();
}

size_t Painter::push_state()
{
    assert(!m_states.empty());
    m_states.emplace_back(m_states.back());
    return m_states.size() - 1;
}

size_t Painter::pop_state()
{
    if (m_states.size() > 1) {
        m_states.pop_back();
    }
    else {
        m_states.back() = State();
    }
    assert(!m_states.empty());
    return m_states.size() - 1;
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
    s_commands.clear();
    m_points.clear();
    m_stylus = Vector2f::zero();
}

void Painter::close_path()
{
    _append_commands({_to_float(Command::CLOSE)});
}

void Painter::set_winding(const Winding winding)
{
    _append_commands({_to_float(Command::WINDING), static_cast<float>(to_number(winding))});
}

void Painter::move_to(const float x, const float y)
{
    _append_commands({_to_float(Command::MOVE), x, y});
}

void Painter::line_to(const float x, const float y)
{
    _append_commands({_to_float(Command::LINE), x, y});
}

void Painter::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    _append_commands({
        _to_float(Command::BEZIER),
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
    _append_commands({_to_float(Command::BEZIER), c1x, c1y, c2x, c2y, tx, ty});
}

void Painter::arc(float cx, float cy, float r, float a0, float a1, Winding dir)
{
    // clamp angles
    float da = a1 - a0;
    if (dir == Winding::CLOCKWISE) {
        if (abs(da) >= TWO_PI) {
            da = TWO_PI;
        }
        else {
            while (da < 0) {
                da += TWO_PI;
            }
        }
    }
    else {
        if (abs(da) <= -TWO_PI) {
            da = -TWO_PI;
        }
        else {
            while (da > 0) {
                da -= TWO_PI;
            }
        }
    }

    // split the arc into <= 90deg segments
    const float ndivs = max(1.f, min(ceilf(abs(da) / HALF_PI), 5.f));
    const float hda   = (da / ndivs) / 2;
    const float kappa = abs(4.0f / 3.0f * (1.0f - cos(hda)) / sin(hda)) * (dir == Winding::CLOCKWISE ? 1 : -1);

    // create individual commands
    std::vector<float> commands((static_cast<size_t>(ceilf(ndivs))) * 7 + 3);
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
            commands[command_index++] = _to_float(s_commands.empty() ? Command::MOVE : Command::LINE);
            commands[command_index++] = x;
            commands[command_index++] = y;
        }
        else {
            commands[command_index++] = _to_float(Command::BEZIER);
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
    if (radius < m_distance_tolerance
        || m_stylus.is_approx(tangent, m_distance_tolerance)
        || tangent.is_approx(end, m_distance_tolerance)
        || Line2::from_points(m_stylus, end).closest_point(tangent).magnitude_sq() < (m_distance_tolerance * m_distance_tolerance)) {
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
        _to_float(Command::MOVE), x, y,
        _to_float(Command::LINE), x, y + h,
        _to_float(Command::LINE), x + w, y + h,
        _to_float(Command::LINE), x + w, y,
        _to_float(Command::CLOSE),
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
    _append_commands({_to_float(Command::MOVE), x, y + ryTL,
                      _to_float(Command::LINE), x, y + h - ryBL,
                      _to_float(Command::BEZIER), x, y + h - ryBL * (1 - KAPPAf), x + rxBL * (1 - KAPPAf), y + h, x + rxBL, y + h,
                      _to_float(Command::LINE), x + w - rxBR, y + h,
                      _to_float(Command::BEZIER), x + w - rxBR * (1 - KAPPAf), y + h, x + w, y + h - ryBR * (1 - KAPPAf), x + w, y + h - ryBR,
                      _to_float(Command::LINE), x + w, y + ryTR,
                      _to_float(Command::BEZIER), x + w, y + ryTR * (1 - KAPPAf), x + w - rxTR * (1 - KAPPAf), y, x + w - rxTR, y,
                      _to_float(Command::LINE), x + rxTL, y,
                      _to_float(Command::BEZIER), x + rxTL * (1 - KAPPAf), y, x, y + ryTL * (1 - KAPPAf), x, y + ryTL,
                      _to_float(Command::CLOSE)});
}

void Painter::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    _append_commands({_to_float(Command::MOVE), cx - rx, cy,
                      _to_float(Command::BEZIER), cx - rx, cy + ry * KAPPAf, cx - rx * KAPPAf, cy + ry, cx, cy + ry,
                      _to_float(Command::BEZIER), cx + rx * KAPPAf, cy + ry, cx + rx, cy + ry * KAPPAf, cx + rx, cy,
                      _to_float(Command::BEZIER), cx + rx, cy - ry * KAPPAf, cx + rx * KAPPAf, cy - ry, cx, cy - ry,
                      _to_float(Command::BEZIER), cx - rx * KAPPAf, cy - ry, cx - rx, cy - ry * KAPPAf, cx - rx, cy,
                      _to_float(Command::CLOSE)});
}

void Painter::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    s_commands.reserve(s_commands.size() + commands.size());

    // commands operate in the context's current transformation space, but we need them in global space
    const Xform2f& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size();) {
        Command command = static_cast<Command>(commands[i]);
        switch (command) {

        case Command::MOVE:
        case Command::LINE: {
            transform_command_pos(commands, i + 1, xform);
            m_stylus = *reinterpret_cast<Vector2f*>(&commands[i + 1]);
            i += 3;
        } break;

        case Command::BEZIER: {
            transform_command_pos(commands, i + 1, xform);
            transform_command_pos(commands, i + 3, xform);
            transform_command_pos(commands, i + 5, xform);
            m_stylus = *reinterpret_cast<Vector2f*>(&commands[i + 5]);
            i += 7;
        } break;

        case Command::WINDING:
            i += 2;
            break;

        case Command::CLOSE:
            i += 1;
            break;

        default:
            assert(0);
        }
    }

    // finally, append the new commands to the existing ones
    std::move(commands.begin(), commands.end(), std::back_inserter(s_commands));
}

} // namespace notf
