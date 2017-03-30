#include "graphics/cell/painterpreter.hpp"

#include "common/vector.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/vertex.hpp"

namespace notf {

void Painterpreter::reset()
{
    m_paths.clear();
    m_points.clear();
    m_commands.clear();
}

void Painterpreter::add_point(const Vector2f position, const Point::Flags flags)
{
    assert(!m_paths.empty());

    // if the point is not significantly different from the last one, use that instead.
    if (!m_points.empty()) {
        Point& last_point = m_points.back();
        if (position.is_approx(last_point.pos, m_distance_tolerance)) {
            last_point.flags = static_cast<Point::Flags>(last_point.flags | flags);
            return;
        }
    }

    // otherwise append create a new point to the current path
    m_points.emplace_back(Point{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});
    m_paths.back().point_count++;
}

float Painterpreter::get_path_area(const Path& path) const
{
    float area     = 0;
    const Point& a = m_points[0];
    for (size_t index = path.first_point + 2; index < path.first_point + path.point_count; ++index) {
        const Point& b = m_points[index - 1];
        const Point& c = m_points[index];
        area += (c.pos.x - a.pos.x) * (b.pos.y - a.pos.y) - (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y);
    }
    return area / 2;
}

void Painterpreter::tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
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

        add_point({px, py}, (t > 0 ? Point::Flags::CORNER : Point::Flags::NONE));

        // advance along the curve.
        t += dt;
        assert(t <= one);
    }
}

std::tuple<float, float, float, float>
Painterpreter::choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width)
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

void Painterpreter::create_bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                                      const float left_u, const float right_u, std::vector<Vertex>& vertices_out)
{
    if (current_point.flags & Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, left_w);

        vertices_out.emplace_back(Vector2f(lx0, ly0),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                           current_point.pos.y + previous_point.forward.x * right_w),
                                  Vector2f(right_u, 1));

        if (current_point.flags & Point::Flags::BEVEL) {
            vertices_out.emplace_back(Vector2f(lx0, ly0),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                               current_point.pos.y + previous_point.forward.x * right_w),
                                      Vector2f(right_u, 1));

            vertices_out.emplace_back(Vector2f(lx1, ly1),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                               current_point.pos.y + current_point.forward.x * right_w),
                                      Vector2f(right_u, 1));
        }
        else {
            const float rx0 = current_point.pos.x - current_point.dm.x * right_w;
            const float ry0 = current_point.pos.y - current_point.dm.y * right_w;

            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));
            vertices_out.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                               current_point.pos.y + previous_point.forward.x * right_w),
                                      Vector2f(right_u, 1));

            vertices_out.emplace_back(Vector2f(rx0, ry0),
                                      Vector2f(right_u, 1));
            vertices_out.emplace_back(Vector2f(rx0, ry0),
                                      Vector2f(right_u, 1));

            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));
            vertices_out.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                               current_point.pos.y + current_point.forward.x * right_w),
                                      Vector2f(right_u, 1));
        }

        vertices_out.emplace_back(Vector2f(lx1, ly1),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * right_w,
                                           current_point.pos.y + current_point.forward.x * right_w),
                                  Vector2f(right_u, 1));
    }

    // right corner
    else {
        float rx0, ry0, rx1, ry1;
        std::tie(rx0, ry0, rx1, ry1) = choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
                                                    previous_point, current_point, -right_w);

        vertices_out.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                           current_point.pos.y - previous_point.forward.x * left_w),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(rx0, ry0),
                                  Vector2f(right_u, 1));

        if (current_point.flags & Point::Flags::BEVEL) {
            vertices_out.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                               current_point.pos.y - previous_point.forward.x * left_w),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(Vector2f(rx0, ry0),
                                      Vector2f(right_u, 1));

            vertices_out.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                               current_point.pos.y - current_point.forward.x * left_w),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(Vector2f(rx1, ry1),
                                      Vector2f(right_u, 1));
        }
        else {
            const float lx0 = current_point.pos.x + current_point.dm.x * left_w;
            const float ly0 = current_point.pos.y + current_point.dm.y * left_w;

            vertices_out.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                               current_point.pos.y - previous_point.forward.x * left_w),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));

            vertices_out.emplace_back(Vector2f(lx0, ly0),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(Vector2f(lx0, ly0),
                                      Vector2f(left_u, 1));

            vertices_out.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                               current_point.pos.y - current_point.forward.x * left_w),
                                      Vector2f(left_u, 1));
            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));
        }

        vertices_out.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * left_w,
                                           current_point.pos.y - current_point.forward.x * left_w),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(rx1, ry1),
                                  Vector2f(right_u, 1));
    }
}

void Painterpreter::create_round_join(const Point& previous_point, const Point& current_point, const float stroke_width,
                                      const size_t divisions, std::vector<Vertex>& vertices_out)
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

        vertices_out.emplace_back(Vector2f(lx0, ly0),
                                  Vector2f(0, 1));
        vertices_out.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * stroke_width,
                                           current_point.pos.y + previous_point.forward.x * stroke_width),
                                  Vector2f(1, 1));

        size_t n = clamp(static_cast<size_t>(ceilf(((a1 - a0) / static_cast<float>(PI)) * divisions)), 2u, divisions);
        for (size_t i = 0; i < n; i++) {
            const float u = i / static_cast<float>(n - 1);
            const float a = a0 + u * (a1 - a0);
            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));
            vertices_out.emplace_back(Vector2f(current_point.pos.x + cosf(a) * stroke_width,
                                               current_point.pos.y + sinf(a) * stroke_width),
                                      Vector2f(1, 1));
        }

        vertices_out.emplace_back(Vector2f(lx1, ly1),
                                  Vector2f(0, 1));
        vertices_out.emplace_back(Vector2f(current_point.pos.x - current_point.forward.y * stroke_width,
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

        vertices_out.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * stroke_width,
                                           current_point.pos.y - previous_point.forward.x * stroke_width),
                                  Vector2f(0, 1));
        vertices_out.emplace_back(Vector2f(rx0, ry0),
                                  Vector2f(1, 1));

        size_t n = clamp(static_cast<size_t>(ceilf(((a1 - a0) / static_cast<float>(PI)) * divisions)), 2u, divisions);
        for (size_t i = 0; i < n; i++) {
            const float u = i / static_cast<float>(n - 1);
            const float a = a0 + u * (a1 - a0);
            vertices_out.emplace_back(Vector2f(current_point.pos.x + cosf(a) * stroke_width,
                                               current_point.pos.y + sinf(a) * stroke_width),
                                      Vector2f(0, 1));
            vertices_out.emplace_back(current_point.pos,
                                      Vector2f(0.5f, 1));
        }

        vertices_out.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * stroke_width,
                                           current_point.pos.y - current_point.forward.x * stroke_width),
                                  Vector2f(0, 1));
        vertices_out.emplace_back(Vector2f(rx1, ry1),
                                  Vector2f(1, 1));
    }
}

void Painterpreter::create_round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width,
                                           const size_t divisions, std::vector<Vertex>& vertices_out)
{
    for (size_t i = 0; i < divisions; i++) {
        const float a  = i / static_cast<float>(divisions - 1) * static_cast<float>(PI);
        const float ax = cos(a) * stroke_width;
        const float ay = sin(a) * stroke_width;
        vertices_out.emplace_back(Vector2f(point.pos.x - delta.y * ax - delta.x * ay,
                                           point.pos.y + delta.x * ax - delta.y * ay),
                                  Vector2f(0, 1));
        vertices_out.emplace_back(point.pos,
                                  Vector2f(.5f, 1.f));
    }

    vertices_out.emplace_back(Vector2f(point.pos.x + delta.y * stroke_width,
                                       point.pos.y - delta.x * stroke_width),
                              Vector2f(0, 1));
    vertices_out.emplace_back(Vector2f(point.pos.x - delta.y * stroke_width,
                                       point.pos.y + delta.x * stroke_width),
                              Vector2f(1, 1));
}

void Painterpreter::create_round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                                         const size_t divisions, std::vector<Vertex>& vertices_out)
{
    vertices_out.emplace_back(Vector2f(point.pos.x + delta.y * stroke_width,
                                       point.pos.y - delta.x * stroke_width),
                              Vector2f(0, 1));
    vertices_out.emplace_back(Vector2f(point.pos.x - delta.y * stroke_width,
                                       point.pos.y + delta.x * stroke_width),
                              Vector2f(1, 1));

    for (size_t i = 0; i < divisions; i++) {
        const float a  = i / static_cast<float>(divisions - 1) * static_cast<float>(PI);
        const float ax = cos(a) * stroke_width;
        const float ay = sin(a) * stroke_width;
        vertices_out.emplace_back(point.pos,
                                  Vector2f(.5f, 1.f));
        vertices_out.emplace_back(Vector2f(point.pos.x - delta.y * ax + delta.x * ay,
                                           point.pos.y + delta.x * ax + delta.y * ay),
                                  Vector2f(0, 1));
    }
}

void Painterpreter::create_butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width,
                                          const float d, const float fringe_width, std::vector<Vertex>& vertices_out) // TODO: what is `d`?
{
    const float px = point.pos.x - (direction.x * d);
    const float py = point.pos.y - (direction.y * d);
    vertices_out.emplace_back(Vector2f(px + direction.y * stroke_width - direction.x * fringe_width,
                                       py + -direction.x * stroke_width - direction.y * fringe_width),
                              Vector2f(0, 0));
    vertices_out.emplace_back(Vector2f(px - direction.y * stroke_width - direction.x * fringe_width,
                                       py - -direction.x * stroke_width - direction.y * fringe_width),
                              Vector2f(1, 0));
    vertices_out.emplace_back(Vector2f(px + direction.y * stroke_width,
                                       py + -direction.x * stroke_width),
                              Vector2f(0, 1));
    vertices_out.emplace_back(Vector2f(px - direction.y * stroke_width,
                                       py - -direction.x * stroke_width),
                              Vector2f(1, 1));
}

void Painterpreter::create_butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                                        const float d, const float fringe_width, std::vector<Vertex>& vertices_out)
{
    const float px = point.pos.x + (delta.x * d);
    const float py = point.pos.y + (delta.y * d);
    vertices_out.emplace_back(Vector2f(px + delta.y * stroke_width,
                                       py - delta.x * stroke_width),
                              Vector2f(0, 1));
    vertices_out.emplace_back(Vector2f(px - delta.y * stroke_width,
                                       py + delta.x * stroke_width),
                              Vector2f(1, 1));
    vertices_out.emplace_back(Vector2f(px + delta.y * stroke_width + delta.x * fringe_width,
                                       py - delta.x * stroke_width + delta.y * fringe_width),
                              Vector2f(0, 0));
    vertices_out.emplace_back(Vector2f(px - delta.y * stroke_width + delta.x * fringe_width,
                                       py + delta.x * stroke_width + delta.y * fringe_width),
                              Vector2f(1, 0));
}

void Painterpreter::_fill(Cell& cell)
{
    const State& state = _get_current_state();

    // get the fill paint
    Paint fill_paint = state.fill;
    fill_paint.inner_color.a *= state.alpha;
    fill_paint.outer_color.a *= state.alpha;

    const float fringe = m_context.provides_geometric_aa() ? m_fringe_width : 0;
    _prepare_paths(cell, fringe, Painter::LineJoin::MITER, 2.4f);

    // create the Cell's render call
    Cell::Call& render_call = create_back(cell.m_calls);
    if (m_paths.size() == 1 && m_paths.front().is_convex) {
        render_call.type = Cell::Call::Type::CONVEX_FILL;
    }
    else {
        render_call.type = Cell::Call::Type::FILL;
    }
    render_call.path_offset  = cell.m_paths.size();
    render_call.paint        = std::move(fill_paint);
    render_call.scissor      = state.scissor;
    render_call.stroke_width = 0;

    const float woff = fringe / 2;
    for (Path& path : m_paths) {
        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        // create the fill vertices
        Cell::Path& cell_path = create_back(cell.m_paths);
        cell_path.fill_offset = cell.m_vertices.size();
        if (fringe > 0) {
            // create a loop
            for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Point& previous_point = m_points[previous_offset];
                const Point& current_point  = m_points[current_offset];

                // no bevel
                if (!(current_point.flags & Point::Flags::BEVEL) || current_point.flags & Point::Flags::LEFT) {
                    cell.m_vertices.emplace_back(current_point.pos + (current_point.dm * woff),
                                                 Vector2f(.5f, 1));
                }

                // beveling requires an extra vertex
                else {
                    cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * woff,
                                                          current_point.pos.y - previous_point.forward.x * woff),
                                                 Vector2f(.5f, 1));
                    cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * woff,
                                                          current_point.pos.y - current_point.forward.x * woff),
                                                 Vector2f(.5f, 1));
                }
            }
        }
        // no fringe = no antialiasing
        else {
            for (size_t point_offset = path.first_point; point_offset <= last_point_offset; ++point_offset) {
                cell.m_vertices.emplace_back(m_points[point_offset].pos,
                                             Vector2f(.5f, 1));
            }
        }
        cell_path.fill_count = cell.m_vertices.size() - cell_path.fill_offset;

        // create stroke vertices, if we draw this shape antialiased
        if (fringe > 0) {
            cell_path.stroke_offset = cell.m_vertices.size();

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
            for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Point& previous_point = m_points[previous_offset];
                const Point& current_point  = m_points[current_offset];

                if (current_point.flags & (Point::Flags::BEVEL | Point::Flags::INNERBEVEL)) {
                    create_bevel_join(previous_point, current_point, left_w, right_w, left_u, right_u, cell.m_vertices);
                }
                else {
                    cell.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.dm.x * left_w,
                                                          current_point.pos.y + current_point.dm.y * left_w),
                                                 Vector2f(left_u, 1));
                    cell.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.dm.x * right_w,
                                                          current_point.pos.y - current_point.dm.y * right_w),
                                                 Vector2f(right_u, 1));
                }
            }

            // copy the first two vertices from the beginning to form a cohesive loop
            cell.m_vertices.emplace_back(cell.m_vertices[cell_path.stroke_offset + 0].pos,
                                         Vector2f(left_u, 1));
            cell.m_vertices.emplace_back(cell.m_vertices[cell_path.stroke_offset + 1].pos,
                                         Vector2f(right_u, 1));

            cell_path.stroke_count = cell.m_vertices.size() - cell_path.stroke_offset;
        }
    }
    render_call.path_count = cell.m_paths.size() - render_call.path_offset;
}

void Painterpreter::_stroke(Cell& cell)
{
    const State& state = _get_current_state();

    // get the stroke paint
    Paint stroke_paint = state.stroke;
    stroke_paint.inner_color.a *= state.alpha;
    stroke_paint.outer_color.a *= state.alpha;

    float stroke_width;
    { // create a sane stroke width
        const float scale = (state.xform.scale_factor_x() + state.xform.scale_factor_y()) / 2;
        stroke_width      = clamp(state.stroke_width * scale, 0, 200); // 200 is arbitrary
        if (stroke_width < m_fringe_width) {
            // if the stroke width is less than pixel size, use alpha to emulate coverage.
            const float alpha = clamp(stroke_width / m_fringe_width, 0.0f, 1.0f);
            stroke_paint.inner_color.a *= alpha * alpha; // since coverage is area, scale by alpha*alpha
            stroke_paint.outer_color.a *= alpha * alpha;
            stroke_width = m_fringe_width;
        }
        //
        if (m_context.provides_geometric_aa()) {
            stroke_width = (stroke_width / 2.f) + (m_fringe_width / 2.f);
        }
        else {
            stroke_width /= 2.f;
        }
    }

    // create the Cell's render call
    Cell::Call& render_call  = create_back(cell.m_calls);
    render_call.type         = Cell::Call::Type::STROKE;
    render_call.path_offset  = cell.m_paths.size();
    render_call.paint        = stroke_paint;
    render_call.scissor      = state.scissor;
    render_call.stroke_width = stroke_width;

    size_t cap_divisions;
    { // calculate divisions per half circle
        float da      = acos(stroke_width / (stroke_width + m_tesselation_tolerance)) * 2;
        cap_divisions = max(size_t(2), static_cast<size_t>(ceilf(static_cast<float>(PI) / da)));
    }

    const Painter::LineJoin line_join = _get_current_state().line_join;
    const Painter::LineCap line_cap   = _get_current_state().line_cap;
    _prepare_paths(cell, stroke_width, line_join, _get_current_state().miter_limit);

    for (Path& path : m_paths) {
        assert(path.point_count > 1);

        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        Cell::Path& cell_path   = create_back(cell.m_paths);
        cell_path.stroke_offset = cell.m_vertices.size();

        size_t previous_offset, current_offset;
        if (path.is_closed) { // loop
            previous_offset = last_point_offset;
            current_offset  = path.first_point;
        }
        else { // capped
            previous_offset = path.first_point + 0;
            current_offset  = path.first_point + 1;
        }

        if (!path.is_closed) {
            // add cap
            switch (line_cap) {
            case Painter::LineCap::BUTT:
                create_butt_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, m_fringe_width / -2, m_fringe_width, cell.m_vertices);
                break;
            case Painter::LineCap::SQUARE:
                create_butt_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, stroke_width - m_fringe_width, m_fringe_width, cell.m_vertices);
                break;
            case Painter::LineCap::ROUND:
                create_round_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, cap_divisions, cell.m_vertices);
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
                if (line_join == Painter::LineJoin::ROUND) {
                    create_round_join(previous_point, current_point, stroke_width, cap_divisions, cell.m_vertices);
                }
                else {
                    create_bevel_join(previous_point, current_point, stroke_width, stroke_width, 0, 1, cell.m_vertices);
                }
            }
            else {
                cell.m_vertices.emplace_back(current_point.pos + (current_point.dm * stroke_width),
                                             Vector2f(0, 1));
                cell.m_vertices.emplace_back(current_point.pos - (current_point.dm * stroke_width),
                                             Vector2f(1, 1));
            }
        }

        if (path.is_closed) {
            // loop it
            cell.m_vertices.emplace_back(cell.m_vertices[cell_path.stroke_offset + 0].pos, Vector2f(0, 1));
            cell.m_vertices.emplace_back(cell.m_vertices[cell_path.stroke_offset + 1].pos, Vector2f(1, 1));
        }
        else {
            // add cap
            switch (line_cap) {
            case Painter::LineCap::BUTT:
                create_butt_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, m_fringe_width / -2, m_fringe_width, cell.m_vertices);
                break;
            case Painter::LineCap::SQUARE:
                create_butt_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, stroke_width - m_fringe_width, m_fringe_width, cell.m_vertices);
                break;
            case Painter::LineCap::ROUND:
                create_round_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, cap_divisions, cell.m_vertices);
                break;
            default:
                assert(0);
            }
        }
        cell_path.stroke_count = cell.m_vertices.size() - cell_path.stroke_offset;
    }
    render_call.path_count = cell.m_paths.size() - render_call.path_offset;
}

void Painterpreter::_prepare_paths(Cell& cell, const float fringe, const Painter::LineJoin join, const float miter_limit)
{
    for (size_t path_index = 0; path_index < m_paths.size(); ++path_index) {
        Path& path = m_paths[path_index];

        // if the first and last points are the same, remove the last one and mark the path as closed
        if (path.point_count >= 2) {
            const Point& first = m_points[path.first_point];
            const Point& last  = m_points[path.first_point + path.point_count - 1];
            if (first.pos.is_approx(last.pos, m_distance_tolerance)) {
                --path.point_count;
                path.is_closed = true;
            }
        }

        // remove paths with just a single point
        if (path.point_count < 2) {
            m_paths.erase(iterator_at(m_paths, path_index));
            --path_index;
            continue;
        }

        // enforce winding
        const float area = get_path_area(path);
        if ((path.winding == Painter::Winding::CCW && area < 0)
            || (path.winding == Painter::Winding::CW && area > 0)) {
            for (size_t i = path.first_point, j = path.first_point + path.point_count - 1; i < j; ++i, --j) {
                std::swap(m_points[i], m_points[j]);
            }
        }

        // make a first pass over all points in the path to determine their `forward` and `length` fields
        const size_t last_point_offset = path.first_point + path.point_count - 1;
        for (size_t next_offset = path.first_point, current_offset = last_point_offset;
             next_offset <= last_point_offset;
             current_offset = next_offset++) {
            Point& current_point = m_points[current_offset];
            Point& next_point    = m_points[next_offset];

            current_point.forward = next_point.pos - current_point.pos;
            current_point.length  = current_point.forward.magnitude();
            if (current_point.length > 0) {
                current_point.forward /= current_point.length;
            }
        }

        // prepare each point of the path
        size_t left_turn_count = 0;
        for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
             current_offset <= last_point_offset;
             previous_offset = current_offset++) {
            const Point& previous_point = m_points[previous_offset];
            Point& current_point        = m_points[current_offset];

            // clear all flags except the corner (in case the Path has already been prepared one)
            current_point.flags = (current_point.flags & Point::Flags::CORNER) ? Point::Flags::CORNER
                                                                               : Point::Flags::NONE;

            // keep track of left turns
            if (current_point.forward.cross(previous_point.forward) > 0) {
                ++left_turn_count;
                current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::LEFT);
            }

            // calculate extrusions
            current_point.dm.x    = (previous_point.forward.y + current_point.forward.y) / 2.f;
            current_point.dm.y    = (previous_point.forward.x + current_point.forward.x) / -2.f;
            const float dm_mag_sq = current_point.dm.magnitude_sq();
            if (dm_mag_sq > precision_low<float>()) {
                float scale = 1.0f / dm_mag_sq;
                if (scale > 600.0f) { // 600 seems to be an arbitrary value?
                    scale = 600.0f;
                }
                current_point.dm.x *= scale;
                current_point.dm.y *= scale;
            }

            // calculate if we should use bevel or miter for inner join
            float limit = max(1.01f, min(previous_point.length, current_point.length) * (fringe > 0.0f ? 1.0f / fringe : 0.f));
            if ((dm_mag_sq * limit * limit) < 1.0f) {
                current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::INNERBEVEL);
            }

            // check to see if the corner needs to be beveled
            if (current_point.flags & Point::Flags::CORNER) {
                if (join == Painter::LineJoin::BEVEL
                    || join == Painter::LineJoin::ROUND
                    || (dm_mag_sq * miter_limit * miter_limit) < 1.0f) {
                    current_point.flags = static_cast<Point::Flags>(current_point.flags | Point::Flags::BEVEL);
                }
            }

            // update the Cell's bounds
            cell.m_bounds._min.x = min(cell.m_bounds._min.x, current_point.pos.x);
            cell.m_bounds._min.y = min(cell.m_bounds._min.y, current_point.pos.y);
            cell.m_bounds._max.x = max(cell.m_bounds._max.x, current_point.pos.x);
            cell.m_bounds._max.y = max(cell.m_bounds._max.y, current_point.pos.y);
        }

        path.is_convex = (left_turn_count == path.point_count);
    }
}

// TODO: when this draws, revisit this function and see if you find a way to guarantee that there is always a path and point without if-statements all over the place
void Painterpreter::_execute(Cell& cell)
{
    // prepare the cell
    cell.m_calls.clear();
    cell.m_paths.clear();
    cell.m_vertices.clear();
    cell.m_bounds = Aabrf::wrongest();

    // parse the command buffer
    for (size_t index = 0; index < m_commands.size(); ++index) {
        switch (static_cast<Command::Type>(m_commands[index])) {

        case Command::Type::MOVE:
            add_path();
            add_point(as_vector2f(m_commands, index + 1), Point::Flags::CORNER);
            index += 2;
            break;

        case Command::LINE:
            if (m_paths.empty()) {
                add_path();
            }
            add_point(as_vector2f(m_commands, index + 1), Point::Flags::CORNER);
            index += 2;
            break;

        case Command::BEZIER:
            if (m_paths.empty()) {
                add_path();
            }
            if (m_points.empty()) {
                add_point(Vector2f::zero(), Point::Flags::CORNER);
            }
            tesselate_bezier(m_points.back().pos.x, m_points.back().pos.y,
                             m_commands[index + 1], m_commands[index + 2],
                             m_commands[index + 3], m_commands[index + 4],
                             m_commands[index + 5], m_commands[index + 6]);
            index += 6;
            break;

        case Command::CLOSE:
            if (!m_paths.empty()) {
                m_paths.back().is_closed = true;
            }
            break;

        case Command::SET_WINDING:
            if (!m_paths.empty()) {
                m_paths.back().winding = static_cast<Painter::Winding>(m_commands[index + 1]);
            }
            index += 1;
            break;

        case Command::BEGIN_PATH:
            m_paths.clear();
            m_points.clear();
            break;

        case Command::FILL:
            _fill(cell);
            break;

        case Command::STROKE:
            _stroke(cell);
            break;

        default: // unknown enum
            assert(0);
        }
    }

    // finish the Cell
    if (!cell.m_bounds.is_valid()) {
        cell.m_bounds = Aabrf::null();
    }
}

} // namespace notf
