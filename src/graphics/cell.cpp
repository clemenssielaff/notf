#include "graphics/cell.hpp"

#include <tuple>

#include "common/float.hpp"
#include "common/line2.hpp"
#include "common/vector.hpp"
#include "graphics/render_context.hpp"

namespace { // anonymous
using namespace notf;

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

static const float KAPPAf = static_cast<float>(KAPPA);

} // namespace anonymous

namespace notf {

Cell_Old::Cell_Old()
    : m_states({RenderState()})
    , m_commands()
    , m_stylus(Vector2f::zero())
{
}

void Cell_Old::reset(const RenderContext& context)
{
    m_states.clear();
    m_states.emplace_back(RenderState());

    const float pixel_ratio = context.get_pixel_ratio();
    m_tesselation_tolerance = 0.25f / pixel_ratio;
    m_distance_tolerance    = 0.01f / pixel_ratio;
    m_fringe_width          = 1.f / pixel_ratio;
}

size_t Cell_Old::push_state()
{
    assert(!m_states.empty());
    m_states.emplace_back(m_states.back());
    return m_states.size() - 1;
}

size_t Cell_Old::pop_state()
{
    if (m_states.size() > 1) {
        m_states.pop_back();
    }
    assert(!m_states.empty());
    return m_states.size() - 1;
}

const Cell_Old::RenderState& Cell_Old::get_current_state() const
{
    assert(!m_states.empty());
    return m_states.back();
}

void Cell_Old::set_stroke_paint(Paint paint)
{
    RenderState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.stroke = std::move(paint);
}

void Cell_Old::set_fill_paint(Paint paint)
{
    RenderState& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.fill = std::move(paint);
}

void Cell_Old::set_scissor(const Aabrf& aabr)
{
    RenderState& current_state  = _get_current_state();
    current_state.scissor.xform = Xform2f::translation(aabr.center());
    current_state.scissor.xform *= current_state.xform;
    current_state.scissor.extend = aabr.extend();
}

void Cell_Old::begin_path()
{
    m_commands.clear();
    m_paths.clear();
    m_points.clear();
    m_vertices.clear();
    m_stylus = Vector2f::zero();
}

void Cell_Old::move_to(const float x, const float y)
{
    _append_commands({to_float(Command::MOVE), std::move(x), std::move(y)});
}

void Cell_Old::line_to(const float x, const float y)
{
    _append_commands({to_float(Command::LINE), std::move(x), std::move(y)});
}

void Cell_Old::_append_commands(std::vector<float>&& commands)
{
    if (commands.empty()) {
        return;
    }
    m_commands.reserve(m_commands.size() + commands.size());

    // commands operate in the context's current transformation space, but we need them in global space
    const Xform2f& xform = _get_current_state().xform;
    for (size_t i = 0; i < commands.size();) {
        Command command = static_cast<Command>(commands[i]);
        switch (command) {

        case Command::MOVE:
        case Command::LINE: {
            m_stylus = as_vector2f(commands, i + 1);
            transform_command_pos(xform, commands, i + 1);
            i += 3;
        } break;

        case Command::BEZIER: {
            m_stylus = as_vector2f(commands, i + 5);
            transform_command_pos(xform, commands, i + 1);
            transform_command_pos(xform, commands, i + 3);
            transform_command_pos(xform, commands, i + 5);
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

void Cell_Old::add_rounded_rect(const float x, const float y, const float w, const float h,
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

void Cell_Old::arc_to(const Vector2f tangent, const Vector2f end, const float radius)
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

void Cell_Old::arc(float cx, float cy, float r, float a0, float a1, Winding dir)
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
            commands[command_index++] = to_float(m_commands.empty() ? Command::MOVE : Command::LINE);
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

void Cell_Old::bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty)
{
    _append_commands({to_float(Command::BEZIER), c1x, c1y, c2x, c2y, tx, ty});
}

void Cell_Old::add_rect(const float x, const float y, const float w, const float h)
{
    _append_commands({
        to_float(Command::MOVE), x, y,
        to_float(Command::LINE), x, y + h,
        to_float(Command::LINE), x + w, y + h,
        to_float(Command::LINE), x + w, y,
        to_float(Command::CLOSE),
    });
}

void Cell_Old::add_ellipse(const float cx, const float cy, const float rx, const float ry)
{
    _append_commands({to_float(Command::MOVE), cx - rx, cy,
                      to_float(Command::BEZIER), cx - rx, cy + ry * KAPPAf, cx - rx * KAPPAf, cy + ry, cx, cy + ry,
                      to_float(Command::BEZIER), cx + rx * KAPPAf, cy + ry, cx + rx, cy + ry * KAPPAf, cx + rx, cy,
                      to_float(Command::BEZIER), cx + rx, cy - ry * KAPPAf, cx + rx * KAPPAf, cy - ry, cx, cy - ry,
                      to_float(Command::BEZIER), cx - rx * KAPPAf, cy - ry, cx - rx, cy - ry * KAPPAf, cx - rx, cy,
                      to_float(Command::CLOSE)});
}

void Cell_Old::quad_to(const float cx, const float cy, const float tx, const float ty)
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

void Cell_Old::set_winding(const Winding winding)
{
    _append_commands({to_float(Command::WINDING), static_cast<float>(to_number(std::move(winding)))});
}

void Cell_Old::close_path()
{
    _append_commands({to_float(Command::CLOSE)});
}

void Cell_Old::fill(RenderContext& context)
{
    const RenderState& state = _get_current_state();
    Paint fill_paint         = state.fill;

    // apply global alpha
    fill_paint.inner_color.a *= state.alpha;
    fill_paint.outer_color.a *= state.alpha;

    _flatten_paths();
    _expand_fill(context.provides_geometric_aa());
    context.add_fill_call(fill_paint, *this);
}

void Cell_Old::stroke(RenderContext& context)
{
    const RenderState& state = _get_current_state();
    Paint stroke_paint       = state.stroke;

    // sanity check the stroke width
    const float scale  = (state.xform.scale_factor_x() + state.xform.scale_factor_y()) / 2;
    float stroke_width = clamp(state.stroke_width * scale, 0, 200); // 200 is arbitrary
    if (stroke_width < m_fringe_width) {
        // if the stroke width is less than pixel size, use alpha to emulate coverage.
        const float alpha = clamp(stroke_width / m_fringe_width, 0.0f, 1.0f);
        stroke_paint.inner_color.a *= alpha * alpha; // since coverage is area, scale by alpha*alpha
        stroke_paint.outer_color.a *= alpha * alpha;
        stroke_width = m_fringe_width;
    }

    // apply global alpha
    stroke_paint.inner_color.a *= state.alpha;
    stroke_paint.outer_color.a *= state.alpha;

    _flatten_paths();
    if (context.provides_geometric_aa()) {
        _expand_stroke((stroke_width / 2.f) + (m_fringe_width / 2.f));
    }
    else {
        _expand_stroke(stroke_width / 2.f);
    }
    context.add_stroke_call(stroke_paint, stroke_width, *this);
}

void Cell_Old::_flatten_paths()
{
    if (!m_paths.empty()) {
        return; // This feels weird and I will change it in the rework discussed at the end of the file
    }

    // parse the command buffer
    for (size_t index = 0; index < m_commands.size();) {
        switch (static_cast<Command>(m_commands[index])) {

        case Command::MOVE:
            m_paths.emplace_back(m_points.size());
            _add_point(*reinterpret_cast<Vector2f*>(&m_commands[index + 1]), Point::Flags::CORNER);
            index += 3;
            break;

        case Command::LINE:
            if(m_paths.empty()){
                m_paths.emplace_back(m_points.size());
            }
            _add_point(*reinterpret_cast<Vector2f*>(&m_commands[index + 1]), Point::Flags::CORNER);
            index += 3;
            break;

        case Command::BEZIER:
            if(m_points.empty()){
                m_points.emplace_back(Point{Vector2f::zero(), Vector2f::zero(), Vector2f::zero(), 0.f, Point::Flags::NONE});
            }
            if(m_paths.empty()){
                m_paths.emplace_back(m_points.size());
            }
            _tesselate_bezier(m_points.back().pos.x, m_points.back().pos.y,
                              m_commands[index + 1], m_commands[index + 2],
                              m_commands[index + 3], m_commands[index + 4],
                              m_commands[index + 5], m_commands[index + 6]);
            index += 7;
            break;

        case Command::CLOSE:
            if (!m_paths.empty()) {
                m_paths.back().is_closed = true;
            }
            ++index;
            break;

        case Command::WINDING:
            if (!m_paths.empty()) {
                m_paths.back().winding = static_cast<Winding>(m_commands[index + 1]);
            }
            index += 2;
            break;

        default: // should never happen
            assert(0);
            ++index;
        }
    }

    m_bounds = Aabrf::wrongest();
    for (size_t path_index = 0; path_index < m_paths.size(); ++path_index) {
        Path& path = m_paths[path_index];

        if (path.point_count >= 2) {
            // if the first and last points are the same, remove the last one and mark the path as closed
            const Point& first = m_points[path.point_offset];
            const Point& last  = m_points[path.point_offset + path.point_count - 1];
            if (first.pos.is_approx(last.pos, m_distance_tolerance)) {
                --path.point_count;
                path.is_closed = true;
            }
        }

        if (path.point_count < 2) {
            // remove paths with just a single point
            m_paths.erase(iterator_at(m_paths, path_index));
            --path_index;
            continue;
        }

        // enforce winding
        const float area = poly_area(m_points, path.point_offset, path.point_count);
        if ((path.winding == Winding::CCW && area < 0)
            || (path.winding == Winding::CW && area > 0)) {
            for (size_t i = path.point_offset, j = path.point_offset + path.point_count - 1; i < j; ++i, --j) {
                std::swap(m_points[i], m_points[j]);
            }
        }

        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        for (size_t next_offset = path.point_offset, current_offset = last_point_offset;
             next_offset <= last_point_offset;
             current_offset = next_offset++) {
            Point& current_point = m_points[current_offset];
            Point& next_point    = m_points[next_offset];

            // calculate segment delta
            current_point.forward = next_point.pos - current_point.pos;
            current_point.length  = current_point.forward.magnitude();
            if (current_point.length > 0) {
                current_point.forward /= current_point.length;
            }

            // update bounds
            m_bounds._min.x = min(m_bounds._min.x, current_point.pos.x);
            m_bounds._min.y = min(m_bounds._min.y, current_point.pos.y);
            m_bounds._max.x = max(m_bounds._max.x, current_point.pos.x);
            m_bounds._max.y = max(m_bounds._max.y, current_point.pos.y);
        }
    }
}

void Cell_Old::_calculate_joins(const float fringe, const LineJoin join, const float miter_limit)
{
    for (Path& path : m_paths) {
        size_t left_turn_count = 0;

        // Calculate which joins needs extra vertices to append, and gather vertex count.
        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
             current_offset <= last_point_offset;
             previous_offset = current_offset++) {
            const Point& previous_point = m_points[previous_offset];
            Point& current_point        = m_points[current_offset];

            // clear all flags except the corner // TODO: this is bullshit, either the points had the chance to collect flags or not, but don't just remove them!
            current_point.flags = (current_point.flags & Point::Flags::CORNER) ? Point::Flags::CORNER : Point::Flags::NONE;

            // keep track of left turns
            if (current_point.forward.cross(previous_point.forward) > 0) {
                ++left_turn_count;
                current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::LEFT);
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
                current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::INNERBEVEL);
            }

            // Check to see if the corner needs to be beveled.
            if (current_point.flags & Point::Flags::CORNER) {
                if (join == LineJoin::BEVEL || join == LineJoin::ROUND || (dm_mag_sq * miter_limit * miter_limit) < 1.0f) {
                    current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::BEVEL);
                }
            }
        }

        path.is_convex = (left_turn_count == path.point_count);
    }
}

void Cell_Old::_expand_fill(const bool draw_antialiased)
{
    const float fringe = draw_antialiased ? m_fringe_width : 0;
    assert(fringe >= 0);

    _calculate_joins(fringe, LineJoin::MITER, 2.4f);

    const float woff = fringe / 2;
    for (Path& path : m_paths) {
        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        // create the fill vertices
        path.fill_offset = m_vertices.size();
        if (fringe > 0) {
            // create a loop
            for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Point& previous_point = m_points[previous_offset];
                const Point& current_point  = m_points[current_offset];

                // no bevel
                if (!(current_point.flags & Point::Flags::BEVEL) || current_point.flags & Point::Flags::LEFT) {
                    m_vertices.emplace_back(current_point.pos + (current_point.dm * woff),
                                            Vector2f(.5f, 1));
                }

                // beveling requires an extra vertex
                else {
                    m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * woff,
                                                     current_point.pos.y - previous_point.forward.x * woff),
                                            Vector2f(.5f, 1));
                    m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * woff,
                                                     current_point.pos.y - current_point.forward.x * woff),
                                            Vector2f(.5f, 1));
                }
            }
        }
        // no fringe = no antialiasing
        else {
            for (size_t point_offset = path.point_offset; point_offset <= last_point_offset; ++point_offset) {
                m_vertices.emplace_back(m_points[point_offset].pos,
                                        Vector2f(.5f, 1));
            }
        }
        path.fill_count = m_vertices.size() - path.fill_offset;

        // create stroke vertices, if we draw this shape antialiased
        if (fringe > 0) {
            path.stroke_offset = m_vertices.size();

            float left_w        = fringe + woff;
            float left_u        = 0;
            const float right_w = fringe - woff;
            const float right_u = 1;

            { // create only half a fringe for convex shapes so that the shape can be rendered without stenciling
                const bool is_convex = m_paths.size() == 1 && m_paths.front().is_convex;
                if (is_convex) {
                    left_w = woff; // this should generate the same vertex as fill inset above
                    left_u = 0.5f; // set outline fade at middle
                }
            }

            // create a loop
            for (size_t current_offset = path.point_offset, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Point& previous_point = m_points[previous_offset];
                const Point& current_point  = m_points[current_offset];

                if (current_point.flags & (Point::Flags::BEVEL | Point::Flags::INNERBEVEL)) {
                    _bevel_join(previous_point, current_point, left_w, right_w, left_u, right_u);
                }
                else {
                    m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.dm.x * left_w,
                                                     current_point.pos.y + current_point.dm.y * left_w),
                                            Vector2f(left_u, 1));
                    m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.dm.x * right_w,
                                                     current_point.pos.y - current_point.dm.y * right_w),
                                            Vector2f(right_u, 1));
                }
            }

            // copy the first two vertices from the beginning to form a cohesive loop
            m_vertices.emplace_back(m_vertices[path.stroke_offset + 0].pos,
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(m_vertices[path.stroke_offset + 1].pos,
                                    Vector2f(right_u, 1));

            path.stroke_count = m_vertices.size() - path.stroke_offset;
        }
    }
}

void Cell_Old::_expand_stroke(const float stroke_width)
{
    size_t cap_count;
    { // calculate divisions per half circle
        float da  = acos(stroke_width / (stroke_width + m_tesselation_tolerance)) * 2;
        cap_count = max(size_t(2), static_cast<size_t>(ceilf(static_cast<float>(PI) / da)));
    }

    const LineJoin line_join = get_current_state().line_join;
    const LineCap line_cap   = get_current_state().line_cap;
    _calculate_joins(stroke_width, line_join, get_current_state().miter_limit);

    for (Path& path : m_paths) {
        assert(path.point_count > 1);

        const size_t last_point_offset = path.point_offset + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        path.stroke_offset = m_vertices.size();

        size_t previous_offset, current_offset;
        if (path.is_closed) { // loop
            previous_offset = last_point_offset;
            current_offset  = path.point_offset;
        }
        else { // capped
            previous_offset = path.point_offset + 0;
            current_offset  = path.point_offset + 1;
        }

        if (!path.is_closed) {
            // add cap
            const Vector2f direction = (m_points[current_offset].pos - m_points[previous_offset].pos).normalize();
            // TODO: direction should be equal to m_points[previous_offset].forward
            switch (line_cap) {
            case LineCap::BUTT:
                _butt_cap_start(m_points[previous_offset], direction, stroke_width, m_fringe_width / -2);
                break;
            case LineCap::SQUARE:
                _butt_cap_start(m_points[previous_offset], direction, stroke_width, stroke_width - m_fringe_width);
                break;
            case LineCap::ROUND:
                _round_cap_start(m_points[previous_offset], direction, stroke_width, cap_count);
                break;
            default:
                assert(0);
            }
        }

        for (; current_offset <= last_point_offset - (path.is_closed ? 0 : 1);
             previous_offset = current_offset++) {
            const Point& previous_point = m_points[previous_offset];
            const Point& current_point  = m_points[current_offset];

            if (current_point.flags & (Point::Flags::BEVEL | Point::Flags::INNERBEVEL)) {
                if (line_join == LineJoin::ROUND) {
                    _round_join(previous_point, current_point, stroke_width, cap_count);
                }
                else {
                    _bevel_join(previous_point, current_point, stroke_width, stroke_width, 0, 1);
                }
            }
            else {
                m_vertices.emplace_back(current_point.pos + (current_point.dm * stroke_width),
                                        Vector2f(0, 1));
                m_vertices.emplace_back(current_point.pos - (current_point.dm * stroke_width),
                                        Vector2f(1, 1));
            }
        }

        if (path.is_closed) {
            // loop it
            m_vertices.emplace_back(m_vertices[path.stroke_offset + 0].pos, Vector2f(0, 1));
            m_vertices.emplace_back(m_vertices[path.stroke_offset + 1].pos, Vector2f(1, 1));
        }
        else {
            // add cap
            const Vector2f delta = (m_points[current_offset].pos - m_points[previous_offset].pos).normalize();
            // TODO: delta should be equal to m_points[previous_offset].forward
            switch (line_cap) {
            case LineCap::BUTT:
                _butt_cap_end(m_points[current_offset], delta, stroke_width, m_fringe_width / -2);
                break;
            case LineCap::SQUARE:
                _butt_cap_end(m_points[current_offset], delta, stroke_width, stroke_width - m_fringe_width);
                break;
            case LineCap::ROUND:
                _round_cap_end(m_points[current_offset], delta, stroke_width, cap_count);
                break;
            default:
                assert(0);
            }
        }

        path.stroke_count = m_vertices.size() - path.stroke_offset;
    }
}

void Cell_Old::_add_point(const Vector2f position, const Point::Flags flags)
{
    // if the point is not significantly different, use the last one instead.
    if (!m_points.empty()) {
        Point& last_point = m_points.back();
        if (position.is_approx(last_point.pos, m_distance_tolerance)) {
            last_point.flags = static_cast<Point::Flags>(last_point.flags | flags);
            return;
        }
    }

    // otherwise create a new point
    m_points.emplace_back(Point{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});

    // and append it to the last path
    assert(!m_paths.empty());
    m_paths.back().point_count++;
}

/* Note that I am using the (experimental) improvement on the standard nanovg tesselation algorithm here,
 * as found at: https://github.com/memononen/nanovg/issues/328
 * If there seems to be an issue with the tesselation, revert back to the "official" implementation
 */
//void Cell::_tesselate_bezier(size_t offset)
void Cell_Old::_tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
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
    const float tol = m_tesselation_tolerance * 4.f;

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

        _add_point({px, py}, (t > 0 ? Point::Flags::CORNER : Point::Flags::NONE));

        // advance along the curve.
        t += dt;
        assert(t <= one);
    }
}

void Cell_Old::_butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width, const float d)
{
    const float px = point.pos.x - direction.x * d;
    const float py = point.pos.y - direction.y * d;
    m_vertices.emplace_back(Vector2f(px + direction.y * stroke_width - direction.x * m_fringe_width,
                                     py + -direction.x * stroke_width - direction.y * m_fringe_width),
                            Vector2f(0, 0));
    m_vertices.emplace_back(Vector2f(px - direction.y * stroke_width - direction.x * m_fringe_width,
                                     py - -direction.x * stroke_width - direction.y * m_fringe_width),
                            Vector2f(1, 0));
    m_vertices.emplace_back(Vector2f(px + direction.y * stroke_width,
                                     py + -direction.x * stroke_width),
                            Vector2f(0, 1));
    m_vertices.emplace_back(Vector2f(px - direction.y * stroke_width,
                                     py - -direction.x * stroke_width),
                            Vector2f(1, 1));
}

void Cell_Old::_butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width, const float d)
{
    const float px = point.pos.x + delta.x * d;
    const float py = point.pos.y + delta.y * d;
    m_vertices.emplace_back(Vector2f(px + delta.y * stroke_width,
                                     py - delta.x * stroke_width),
                            Vector2f(0, 1));
    m_vertices.emplace_back(Vector2f(px - delta.y * stroke_width,
                                     py + delta.x * stroke_width),
                            Vector2f(1, 1));
    m_vertices.emplace_back(Vector2f(px + delta.y * stroke_width + delta.x * m_fringe_width,
                                     py - delta.x * stroke_width + delta.y * m_fringe_width),
                            Vector2f(0, 0));
    m_vertices.emplace_back(Vector2f(px - delta.y * stroke_width + delta.x * m_fringe_width,
                                     py + delta.x * stroke_width + delta.y * m_fringe_width),
                            Vector2f(1, 0));
}

void Cell_Old::_round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width, const size_t cap_count)
{
    for (size_t i = 0; i < cap_count; i++) {
        const float a  = i / static_cast<float>(cap_count - 1) * static_cast<float>(PI);
        const float ax = cos(a) * stroke_width;
        const float ay = sin(a) * stroke_width;
        m_vertices.emplace_back(Vector2f(point.pos.x - delta.y * ax - delta.x * ay,
                                         point.pos.y + delta.x * ax - delta.y * ay),
                                Vector2f(0, 1));
        m_vertices.emplace_back(point.pos,
                                Vector2f(.5f, 1.f));
    }

    m_vertices.emplace_back(Vector2f(point.pos.x + delta.y * stroke_width,
                                     point.pos.y - delta.x * stroke_width),
                            Vector2f(0, 1));
    m_vertices.emplace_back(Vector2f(point.pos.x - delta.y * stroke_width,
                                     point.pos.y + delta.x * stroke_width),
                            Vector2f(1, 1));
}

void Cell_Old::_round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width, const size_t cap_count)
{
    m_vertices.emplace_back(Vector2f(point.pos.x + delta.y * stroke_width,
                                     point.pos.y - delta.x * stroke_width),
                            Vector2f(0, 1));
    m_vertices.emplace_back(Vector2f(point.pos.x - delta.y * stroke_width,
                                     point.pos.y + delta.x * stroke_width),
                            Vector2f(1, 1));

    for (size_t i = 0; i < cap_count; i++) {
        const float a  = i / static_cast<float>(cap_count - 1) * static_cast<float>(PI);
        const float ax = cos(a) * stroke_width;
        const float ay = sin(a) * stroke_width;
        m_vertices.emplace_back(point.pos,
                                Vector2f(.5f, 1.f));
        m_vertices.emplace_back(Vector2f(point.pos.x - delta.y * ax + delta.x * ay,
                                         point.pos.y + delta.x * ax + delta.y * ay),
                                Vector2f(0, 1));
    }
}

void Cell_Old::_bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                       const float left_u, const float right_u)
{
    if (current_point.flags & Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, left_w);

        m_vertices.emplace_back(Vector2f(lx0, ly0),
                                Vector2f(left_u, 1));
        m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                         current_point.pos.y + previous_point.forward.x * right_w),
                                Vector2f(right_u, 1));

        if (current_point.flags & Point::Flags::BEVEL) {
            m_vertices.emplace_back(Vector2f(lx0, ly0),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                             current_point.pos.y + previous_point.forward.x * right_w),
                                    Vector2f(right_u, 1));

            m_vertices.emplace_back(Vector2f(lx1, ly1),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                             current_point.pos.y + current_point.forward.x * right_w),
                                    Vector2f(right_u, 1));
        }
        else {
            const float rx0 = current_point.pos.x - current_point.dm.x * right_w;
            const float ry0 = current_point.pos.y - current_point.dm.y * right_w;

            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));
            m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                             current_point.pos.y + previous_point.forward.x * right_w),
                                    Vector2f(right_u, 1));

            m_vertices.emplace_back(Vector2f(rx0, ry0),
                                    Vector2f(right_u, 1));
            m_vertices.emplace_back(Vector2f(rx0, ry0),
                                    Vector2f(right_u, 1));

            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));
            m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                             current_point.pos.y + current_point.forward.x * right_w),
                                    Vector2f(right_u, 1));
        }

        m_vertices.emplace_back(Vector2f(lx1, ly1),
                                Vector2f(left_u, 1));
        m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                         current_point.pos.y + current_point.forward.x * right_w),
                                Vector2f(right_u, 1));
    }

    // right corner
    else {
        float rx0, ry0, rx1, ry1;
        std::tie(rx0, ry0, rx1, ry1) = choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, -right_w);

        m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                         current_point.pos.y - previous_point.forward.x * left_w),
                                Vector2f(left_u, 1));
        m_vertices.emplace_back(Vector2f(rx0, ry0),
                                Vector2f(right_u, 1));

        if (current_point.flags & Point::Flags::BEVEL) {
            m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                             current_point.pos.y - previous_point.forward.x * left_w),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(Vector2f(rx0, ry0),
                                    Vector2f(right_u, 1));

            m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                             current_point.pos.y - current_point.forward.x * left_w),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(Vector2f(rx1, ry1),
                                    Vector2f(right_u, 1));
        }
        else {
            const float lx0 = current_point.pos.x + current_point.dm.x * left_w;
            const float ly0 = current_point.pos.y + current_point.dm.y * left_w;

            m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                             current_point.pos.y - previous_point.forward.x * left_w),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));

            m_vertices.emplace_back(Vector2f(lx0, ly0),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(Vector2f(lx0, ly0),
                                    Vector2f(left_u, 1));

            m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                             current_point.pos.y - current_point.forward.x * left_w),
                                    Vector2f(left_u, 1));
            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));
        }

        m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                         current_point.pos.y - current_point.forward.x * left_w),
                                Vector2f(left_u, 1));
        m_vertices.emplace_back(Vector2f(rx1, ry1),
                                Vector2f(right_u, 1));
    }
}

void Cell_Old::_round_join(const Point& previous_point, const Point& current_point, const float stroke_width, const size_t ncap)
{
    if (current_point.flags & Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(
            current_point.flags & Point::Flags::INNERBEVEL, previous_point, current_point, stroke_width);
        float a0 = atan2(previous_point.forward.x, -previous_point.forward.y);
        float a1 = atan2(current_point.forward.x, -current_point.forward.y);
        if (a1 > a0) {
            a1 -= TWO_PI;
        }

        m_vertices.emplace_back(Vector2f(lx0, ly0),
                                Vector2f(0, 1));
        m_vertices.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * stroke_width,
                                         current_point.pos.y + previous_point.forward.x * stroke_width),
                                Vector2f(1, 1));

        size_t n = clamp(static_cast<size_t>(ceilf(((a1 - a0) / static_cast<float>(PI)) * ncap)), 2u, ncap);
        for (size_t i = 0; i < n; i++) {
            const float u = i / static_cast<float>(n - 1);
            const float a = a0 + u * (a1 - a0);
            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));
            m_vertices.emplace_back(Vector2f(current_point.pos.x + cosf(a) * stroke_width,
                                             current_point.pos.y + sinf(a) * stroke_width),
                                    Vector2f(1, 1));
        }

        m_vertices.emplace_back(Vector2f(lx1, ly1),
                                Vector2f(0, 1));
        m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * stroke_width,
                                         current_point.pos.y + current_point.forward.x * stroke_width),
                                Vector2f(1, 1));
    }
    else {
        float rx0, ry0, rx1, ry1;
        std::tie(rx0, ry0, rx1, ry1) = choose_bevel(
            current_point.flags & Point::Flags::INNERBEVEL, previous_point, current_point, -stroke_width);
        float a0 = atan2(-previous_point.forward.x, previous_point.forward.y);
        float a1 = atan2(-current_point.forward.x, current_point.forward.y);
        if (a1 < a0) {
            a1 += TWO_PI;
        }

        m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * stroke_width,
                                         current_point.pos.y - previous_point.forward.x * stroke_width),
                                Vector2f(0, 1));
        m_vertices.emplace_back(Vector2f(rx0, ry0),
                                Vector2f(1, 1));

        size_t n = clamp(static_cast<size_t>(ceilf(((a1 - a0) / static_cast<float>(PI)) * ncap)), 2u, ncap);
        for (size_t i = 0; i < n; i++) {
            const float u = i / static_cast<float>(n - 1);
            const float a = a0 + u * (a1 - a0);
            m_vertices.emplace_back(Vector2f(current_point.pos.x + cosf(a) * stroke_width,
                                             current_point.pos.y + sinf(a) * stroke_width),
                                    Vector2f(0, 1));
            m_vertices.emplace_back(current_point.pos,
                                    Vector2f(0.5f, 1));
        }

        m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * stroke_width,
                                         current_point.pos.y - current_point.forward.x * stroke_width),
                                Vector2f(0, 1));
        m_vertices.emplace_back(Vector2f(rx1, ry1),
                                Vector2f(1, 1));
    }
}

float Cell_Old::poly_area(const std::vector<Point>& points, const size_t offset, const size_t count)
{
    float area           = 0;
    const Cell_Old::Point& a = points[0];
    for (size_t index = offset + 2; index < offset + count; ++index) {
        const Cell_Old::Point& b = points[index - 1];
        const Cell_Old::Point& c = points[index];
        area += (c.pos.x - a.pos.x) * (b.pos.y - a.pos.y) - (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y);
    }
    return area / 2;
}

std::tuple<float, float, float, float>
Cell_Old::choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width)
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

/** Transforms a Command to a float value that can be stored in the Command buffer. */
float Cell_Old::to_float(const Command command) { return static_cast<float>(to_number(command)); }

} // namespace notf

/*
 * Unlike NanovVG, which must be very general by design, I want the NoTF canvas layer to be optimized for drawing
 * Widgets from Python.
 * Therefore, I propose the following:
 *  1. Every time a Cell is redrawn, it clears all of its Commands and paths
 *  2. The draw calls, that are exposed to Python, ONLY append to the Command buffer which should be pretty fast
 *     Unlike NanoVG, calling `fill` or `stroke` only inserts a new Command, as does `begin_path`
 *  3. After the Cell is finished redrawing, the Command buffer is optimized.
 *     If a Path is created, then filled, then the path is added (not the old one not removed) and then filled again,
 *     the first fill command is transformed in a NOOP Command which will be ignored - same goes for stroke
 *  3a)... or we keep track of the last `fill` and `stroke` Commands and reset them every time a new one is added
 *     and whenever a `begin_path` Command is given, the two trackers are reset to an invalid value
 *  4. Finally, the Commands are executed and the resulting "paths" (or buffers or whatever) stored in the Cell.
 *  5. Whenever the Widget's Cell is drawn on screen, the Cell's buffers are simply copied to OpenGL.
 *     This way, we only update the Cell's buffers whenever there is something to update and have as little computation
 *     in Python as possible.
 */
