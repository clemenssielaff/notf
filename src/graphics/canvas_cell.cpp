#include "graphics/canvas_cell.hpp"

#include "graphics/canvas_layer.hpp"

namespace { // anonymous
using namespace notf;

void transform_command_point(const Transform2& xform, std::vector<float>& commands, size_t index)
{
    assert(commands.size() >= index + 2);
    Vector2& point = *reinterpret_cast<Vector2*>(&commands[index]);
    point          = xform.transform(point);
}

float to_float(const Cell::Command command) { return static_cast<float>(to_number(command)); }

} // namespace anonymous

namespace notf {

Cell::Cell()
    : m_states()
    , m_commands(256)
    , m_current_command(0)
    , m_stylus(Vector2::zero())
{
}

void Cell::reset(const CanvasLayer& layer)
{
    m_states.clear();
    m_states.emplace_back(RenderState());

    const float pixel_ratio = layer.get_pixel_ratio();
    m_tesselation_tolerance = 0.25f / pixel_ratio;
    m_distance_tolerance    = 0.01f / pixel_ratio;
    m_fringe_width          = 1.0f / pixel_ratio;
}

size_t Cell::push_state()
{
    assert(!m_states.empty());
    m_states.emplace_back(m_states.back());
    return m_states.size() - 1;
}

size_t Cell::pop_state()
{
    if (m_states.size() > 1) {
        m_states.pop_back();
    }
    assert(!m_states.empty());
    return m_states.size() - 1;
}

const Cell::RenderState& Cell::get_current_state() const
{
    assert(!m_states.empty());
    return m_states.back();
}

void Cell::set_stroke_paint(Paint paint)
{
    RenderState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.stroke = std::move(paint);
}

void Cell::set_fill_paint(Paint paint)
{
    RenderState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.fill = std::move(paint);
}

void Cell::set_scissor(const Aabr& aabr)
{
    RenderState& current_state  = _get_current_state();
    current_state.scissor.xform = Transform2::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend() / 2;
}

void Cell::begin_path()
{
    m_commands.clear();
    m_paths.clear();
}

void Cell::move_to(const float x, const float y)
{
    _append_commands({to_float(Command::MOVE), std::move(x), std::move(y)});
}

void Cell::line_to(const float x, const float y)
{
    _append_commands({to_float(Command::LINE), std::move(x), std::move(y)});
}

void Cell::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    m_commands.reserve(m_commands.size() + commands.size());

    // extract the last position of the stylus
    Command command = static_cast<Command>(m_commands[0]);
    if (command != Command::WINDING && command != Command::CLOSE) {
        assert(commands.size() >= 3);
        m_stylus = *reinterpret_cast<Vector2*>(&commands[commands.size() - 2]);
    }

    // commands operate in the context's current transformation space, but we need them in global space
    const Transform2& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size();) {
        command = static_cast<Command>(m_commands[i]);
        switch (command) {

        case Command::MOVE:
        case Command::LINE: {
            transform_command_point(xform, commands, i + 1);
            i += 3;
        } break;

        case Command::BEZIER: {
            transform_command_point(xform, commands, i + 1);
            transform_command_point(xform, commands, i + 3);
            transform_command_point(xform, commands, i + 5);
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
    std::move(commands.begin(), commands.end(), std::back_inserter(m_commands));
}

void Cell::add_rounded_rect(const float x, const float y, const float w, const float h,
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
                      to_float(Command::BEZIER), x, y + h - ryBL * (1 - KAPPA), x + rxBL * (1 - KAPPA), y + h, x + rxBL, y + h,
                      to_float(Command::LINE), x + w - rxBR, y + h,
                      to_float(Command::BEZIER), x + w - rxBR * (1 - KAPPA), y + h, x + w, y + h - ryBR * (1 - KAPPA), x + w, y + h - ryBR,
                      to_float(Command::LINE), x + w, y + ryTR,
                      to_float(Command::BEZIER), x + w, y + ryTR * (1 - KAPPA), x + w - rxTR * (1 - KAPPA), y, x + w - rxTR, y,
                      to_float(Command::LINE), x + rxTL, y,
                      to_float(Command::BEZIER), x + rxTL * (1 - KAPPA), y, x, y + ryTL * (1 - KAPPA), x, y + ryTL,
                      to_float(Command::CLOSE)});
}

void Cell::bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty)
{
    _append_commands({to_float(Command::BEZIER), c1x, c1y, c2x, c2y, tx, ty});
}

void Cell::add_rect(const float x, const float y, const float w, const float h)
{
    _append_commands({
        to_float(Command::MOVE), x, y,
        to_float(Command::LINE), x, y + h,
        to_float(Command::LINE), x + w, y + h,
        to_float(Command::LINE), x + w, y,
        to_float(Command::CLOSE),
    });
}

void Cell::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    _append_commands({to_float(Command::MOVE), cx - rx, cy,
                      to_float(Command::BEZIER), cx - rx, cy + ry * KAPPA, cx - rx * KAPPA, cy + ry, cx, cy + ry,
                      to_float(Command::BEZIER), cx + rx * KAPPA, cy + ry, cx + rx, cy + ry * KAPPA, cx + rx, cy,
                      to_float(Command::BEZIER), cx + rx, cy - ry * KAPPA, cx + rx * KAPPA, cy - ry, cx, cy - ry,
                      to_float(Command::BEZIER), cx - rx * KAPPA, cy - ry, cx - rx, cy - ry * KAPPA, cx - rx, cy,
                      to_float(Command::CLOSE)});
}

void Cell::quad_to(const float cx, const float cy, const float tx, const float ty)
{
    // in order to construct a quad spline with a bezier command we need the position of the last point
    // to infer, where the ctrl points for the bezier is located
    // this works, at the moment, as in nanovg, by storing the (local) position of the stylus, but that's a crutch
    // ideally, there should be a Command::QUAD, if we really need this function
    const float x0 = m_stylus.x;
    const float y0 = m_stylus.y;
    _append_commands({
        to_float(Command::BEZIER),
        x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0),
        tx + 2.0f / 3.0f * (cx - tx), ty + 2.0f / 3.0f * (cy - ty),
        tx, ty,
    });
}

void Cell::set_winding(const Winding winding)
{
    _append_commands({to_float(Command::WINDING), static_cast<float>(to_number(std::move(winding)))});
}

void Cell::close_path()
{
    _append_commands({to_float(Command::CLOSE)});
}

Paint Cell::create_linear_gradient(const Vector2& start_pos, const Vector2& end_pos,
                                   const Color start_color, const Color end_color)
{
    static const float large_number = 1e5;

    Vector2 delta   = end_pos - start_pos;
    const float mag = delta.magnitude();
    if (mag == approx(0, 0.0001f)) {
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
    paint.xform[2][2]   = start_pos.y - (delta.y * large_number);
    paint.radius        = 0.0f;
    paint.feather       = max(1.0f, mag);
    paint.extent.width  = large_number;
    paint.extent.height = large_number + (mag / 2);
    paint.innerColor    = std::move(start_color);
    paint.outerColor    = std::move(end_color);
    return paint;
}

Paint Cell::create_radial_gradient(const Vector2& center,
                                   const float inner_radius, const float outer_radius,
                                   const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Transform2::translation(center);
    paint.radius        = (inner_radius + outer_radius) * 0.5f;
    paint.feather       = max(1.f, outer_radius - inner_radius);
    paint.extent.width  = paint.radius;
    paint.extent.height = paint.radius;
    paint.innerColor    = std::move(inner_color);
    paint.outerColor    = std::move(outer_color);
    return paint;
}

Paint Cell::create_box_gradient(const Vector2& center, const Size2f& extend,
                                const float radius, const float feather,
                                const Color inner_color, const Color outer_color)
{
    Paint paint;
    paint.xform         = Transform2::translation({center.x + extend.width / 2, center.y + extend.height / 2});
    paint.radius        = radius;
    paint.feather       = max(1.f, feather);
    paint.extent.width  = extend.width / 2;
    paint.extent.height = extend.height / 2;
    paint.innerColor    = std::move(inner_color);
    paint.outerColor    = std::move(outer_color);
    return paint;
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Cell::Command) == sizeof(float),
              "Floats on your system don't seem be to be 32 bits wide. "
              "Adjust the type of the underlying type of CommandBuffer::Command to fit your particular system.");

} // namespace notf
