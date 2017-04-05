#include "graphics/cell/painterpreter.hpp"

#include "common/vector.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/cell/commands.hpp"
#include "graphics/render_context.hpp"
#include "graphics/texture2.hpp"
#include "graphics/vertex.hpp"

namespace { // anonymous
using namespace notf;
void xformToMat3x4(float* m3, Xform2f t)
{
    m3[0]  = t[0][0];
    m3[1]  = t[0][1];
    m3[2]  = 0.0f;
    m3[3]  = 0.0f;
    m3[4]  = t[1][0];
    m3[5]  = t[1][1];
    m3[6]  = 0.0f;
    m3[7]  = 0.0f;
    m3[8]  = t[2][0];
    m3[9]  = t[2][1];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

} // namespace anonymous

namespace notf {

void paint_to_frag(RenderContext::ShaderVariables& frag, const Paint& paint, const Scissor& scissor,
                   const float stroke_width, const float fringe, const float stroke_threshold);

Painterpreter::Painterpreter(RenderContext& context)
    : m_context(context)
    , m_points()
    , m_paths()
    , m_states()
    , m_bounds(Aabrf::null())
{
}

// TODO: when this draws, revisit this function and see if you find a way to guarantee that there is always a path and point without if-statements all over the place
// TODO: I'm pretty sure when writing vertices from Cell to RenderContext you'll need to xform them
void Painterpreter::paint(Cell& cell)
{
    _reset();

    // parse the Cell's command buffer
    const PainterCommandBuffer& commands = cell.get_commands();
    for (size_t index = 0; index < commands.size();) {
        switch (PainterCommand::command_t_to_type(commands[index])) {

        case PainterCommand::Type::PUSH_STATE: {
            _push_state();
            index += command_size<PushStateCommand>();
        } break;

        case PainterCommand::Type::POP_STATE: {
            _pop_state();
            index += command_size<PopStateCommand>();
        } break;

        case PainterCommand::BEGIN_PATH: {
            m_paths.clear();
            m_points.clear();
            m_bounds = Aabrf::wrongest();
            index += command_size<BeginCommand>();
        } break;

        case PainterCommand::SET_WINDING: {
            if (!m_paths.empty()) {
                m_paths.back().winding = static_cast<Painter::Winding>(commands[index + 1]);
            }
            index += command_size<SetWindingCommand>();
        } break;

        case PainterCommand::CLOSE: {
            if (!m_paths.empty()) {
                m_paths.back().is_closed = true;
            }
            index += command_size<CloseCommand>();
        } break;

        case PainterCommand::Type::MOVE: {
            _add_path();
            const MoveCommand& cmd = map_command<MoveCommand>(commands, index);
            _add_point(cmd.pos, Point::Flags::CORNER);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::LINE: {
            if (m_paths.empty()) {
                _add_path();
            }
            const LineCommand& cmd = map_command<LineCommand>(commands, index);
            _add_point(cmd.pos, Point::Flags::CORNER);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::BEZIER: {
            if (m_paths.empty()) {
                _add_path();
            }
            Vector2f stylus;
            if (m_points.empty()) {
                stylus = Vector2f::zero();
                _add_point(stylus, Point::Flags::CORNER);
            }
            else {
                stylus = _get_current_state().xform.get_inverse().transform({m_points.back().pos.x, m_points.back().pos.y}); // TODO: that sucks
            }
            const BezierCommand& cmd = map_command<BezierCommand>(commands, index);
            _tesselate_bezier(stylus.x, stylus.y,
                              cmd.ctrl1.x, cmd.ctrl1.y,
                              cmd.ctrl2.x, cmd.ctrl2.y,
                              cmd.end.x, cmd.end.y);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::FILL: {
            _fill();
            index += command_size<FillCommand>();
        } break;

        case PainterCommand::STROKE: {
            _stroke();
            index += command_size<StrokeCommand>();
        } break;

        case PainterCommand::SET_XFORM: {
            const SetXformCommand& cmd = map_command<SetXformCommand>(commands, index);
            _get_current_state().xform = cmd.xform;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::RESET_XFORM: {
            _get_current_state().xform = Xform2f::identity();
            index += command_size<ResetXformCommand>();
        } break;

        case PainterCommand::TRANSFORM: {
            const TransformCommand& cmd = map_command<TransformCommand>(commands, index);
            _get_current_state().xform *= cmd.xform;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::TRANSLATE: {
            const TranslationCommand& cmd = map_command<TranslationCommand>(commands, index);
            _get_current_state().xform *= Xform2f::translation(cmd.delta);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::ROTATE: {
            const RotationCommand& cmd = map_command<RotationCommand>(commands, index);
            _get_current_state().xform = Xform2f::rotation(cmd.angle) * _get_current_state().xform;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_SCISSOR: {
            const SetScissorCommand& cmd = map_command<SetScissorCommand>(commands, index);
            _get_current_state().scissor = cmd.sissor;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::RESET_SCISSOR: {
            const ResetScissorCommand& cmd = map_command<ResetScissorCommand>(commands, index);
            _get_current_state().scissor   = {Xform2f::identity(), {-1, -1}};
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_FILL_COLOR: {
            const FillColorCommand& cmd = map_command<FillColorCommand>(commands, index);
            _get_current_state().fill_paint.set_color(cmd.color);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_FILL_PAINT: {
            const FillPaintCommand& cmd     = map_command<FillPaintCommand>(commands, index);
            _get_current_state().fill_paint = cmd.paint;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_STROKE_COLOR: {
            const StrokeColorCommand& cmd = map_command<StrokeColorCommand>(commands, index);
            _get_current_state().stroke_paint.set_color(cmd.color);
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_STROKE_PAINT: {
            const StrokePaintCommand& cmd     = map_command<StrokePaintCommand>(commands, index);
            _get_current_state().stroke_paint = cmd.paint;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_STROKE_WIDTH: {
            const StrokeWidthCommand& cmd     = map_command<StrokeWidthCommand>(commands, index);
            _get_current_state().stroke_width = cmd.stroke_width;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_BLEND_MODE: {
            const BlendModeCommand& cmd     = map_command<BlendModeCommand>(commands, index);
            _get_current_state().blend_mode = cmd.blend_mode;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_ALPHA: {
            const SetAlphaCommand& cmd = map_command<SetAlphaCommand>(commands, index);
            _get_current_state().alpha = cmd.alpha;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_MITER_LIMIT: {
            const MiterLimitCommand& cmd     = map_command<MiterLimitCommand>(commands, index);
            _get_current_state().miter_limit = cmd.miter_limit;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_LINE_CAP: {
            const LineCapCommand& cmd     = map_command<LineCapCommand>(commands, index);
            _get_current_state().line_cap = cmd.line_cap;
            index += command_size<decltype(cmd)>();
        } break;

        case PainterCommand::SET_LINE_JOIN: {
            const LineJoinCommand& cmd     = map_command<LineJoinCommand>(commands, index);
            _get_current_state().line_join = cmd.line_join;
            index += command_size<decltype(cmd)>();
        } break;

        default:     // unknown enum
            ++index; // failsave
            throw std::runtime_error("Encountered Unknown enum in Painterpreter::paint");
        }
    }
}

void Painterpreter::_reset()
{
    m_paths.clear();
    m_points.clear();

    m_states.clear();
    m_states.emplace_back(); // always have at least one state

    m_bounds = Aabrf::wrongest();
}

void Painterpreter::_push_state()
{
    m_states.emplace_back(m_states.back());
}

void Painterpreter::_pop_state()
{
    assert(m_states.size() > 1);
    m_states.pop_back();
}

void Painterpreter::_add_point(Vector2f position, const Point::Flags flags)
{
    assert(!m_paths.empty());

    // bring the Point from Painter- into Cell-space
    position = _get_current_state().xform.transform(position);

    // if the Point is not significantly different from the last one, use that instead.
    if (!m_points.empty()) {
        Point& last_point = m_points.back();
        if (position.is_approx(last_point.pos, m_context.get_distance_tolerance())) {
            last_point.flags = static_cast<Point::Flags>(last_point.flags | flags);
            return;
        }
    }

    // otherwise append a new Point to the current Path
    m_points.emplace_back(Point{std::move(position), Vector2f::zero(), Vector2f::zero(), 0.f, flags});
    m_paths.back().point_count++;
}

void Painterpreter::_add_path()
{
    Path& new_path       = create_back(m_paths);
    new_path.first_point = m_points.size();
}

float Painterpreter::_get_path_area(const Path& path) const
{
    float area     = 0;
    const Point& a = m_points[path.first_point];
    for (size_t index = path.first_point + 2; index < path.first_point + path.point_count; ++index) {
        const Point& b = m_points[index - 1];
        const Point& c = m_points[index];
        area += (c.pos.x - a.pos.x) * (b.pos.y - a.pos.y) - (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y);
    }
    return area / 2;
}

void Painterpreter::_tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
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

        _add_point({px, py}, (t > 0 ? Point::Flags::CORNER : Point::Flags::NONE));

        // advance along the curve.
        t += dt;
        assert(t <= one);
    }
}

std::tuple<float, float, float, float>
Painterpreter::_choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width)
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

void Painterpreter::_create_bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                                       const float left_u, const float right_u, std::vector<Vertex>& vertices_out)
{
    if (current_point.flags & Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = _choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
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
        std::tie(rx0, ry0, rx1, ry1) = _choose_bevel(current_point.flags & Point::Flags::INNERBEVEL,
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

void Painterpreter::_create_round_join(const Point& previous_point, const Point& current_point, const float stroke_width,
                                       const size_t divisions, std::vector<Vertex>& vertices_out)
{
    if (current_point.flags & Point::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = _choose_bevel(
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
        std::tie(rx0, ry0, rx1, ry1) = _choose_bevel(
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

void Painterpreter::_create_round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width,
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

void Painterpreter::_create_round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
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

void Painterpreter::_create_butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width,
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

void Painterpreter::_create_butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
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

void Painterpreter::_fill()
{
    const detail::PainterState& state = _get_current_state();

    // get the fill paint
    Paint fill_paint = state.fill_paint;
    fill_paint.inner_color.a *= state.alpha;
    fill_paint.outer_color.a *= state.alpha;

    const float fringe_width = m_context.provides_geometric_aa() ? m_context.get_fringe_width() : 0;
    _prepare_paths(fringe_width, Painter::LineJoin::MITER, 2.4f);

    // create the render call
    RenderContext::Call& render_call = create_back(m_context.m_calls);
    if (m_paths.size() == 1 && m_paths.front().is_convex) {
        render_call.type = RenderContext::Call::Type::CONVEX_FILL;
    }
    else {
        render_call.type = RenderContext::Call::Type::FILL;
    }
    render_call.path_offset = m_context.m_paths.size();
    render_call.path_count  = m_paths.size();
    render_call.texture     = _get_current_state().fill_paint.texture;

    const float woff = fringe_width / 2;
    for (Path& path : m_paths) {
        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        // create the fill vertices
        RenderContext::Path& render_path = create_back(m_context.m_paths);
        assert(m_context.m_vertices.size() < std::numeric_limits<GLint>::max());
        render_path.fill_offset = static_cast<GLint>(m_context.m_vertices.size());
        if (fringe_width > 0) {
            // create a loop
            for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const Point& previous_point = m_points[previous_offset];
                const Point& current_point  = m_points[current_offset];

                // no bevel
                if (!(current_point.flags & Point::Flags::BEVEL) || current_point.flags & Point::Flags::LEFT) {
                    m_context.m_vertices.emplace_back(current_point.pos + (current_point.dm * woff),
                                                      Vector2f(.5f, 1));
                }

                // beveling requires an extra vertex
                else {
                    m_context.m_vertices.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * woff,
                                                               current_point.pos.y - previous_point.forward.x * woff),
                                                      Vector2f(.5f, 1));
                    m_context.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.forward.y * woff,
                                                               current_point.pos.y - current_point.forward.x * woff),
                                                      Vector2f(.5f, 1));
                }
            }
        }
        // no fringe = no antialiasing
        else {
            for (size_t point_offset = path.first_point; point_offset <= last_point_offset; ++point_offset) {
                m_context.m_vertices.emplace_back(m_points[point_offset].pos,
                                                  Vector2f(.5f, 1));
            }
        }
        render_path.fill_count = static_cast<GLsizei>(m_context.m_vertices.size() - static_cast<size_t>(render_path.fill_offset));

        // create stroke vertices, if we draw this shape antialiased
        if (fringe_width > 0) {
            assert(m_context.m_vertices.size() < std::numeric_limits<GLint>::max());
            render_path.stroke_offset = static_cast<GLint>(m_context.m_vertices.size());

            float left_w        = fringe_width + woff;
            float left_u        = 0;
            const float right_w = fringe_width - woff;
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
                    _create_bevel_join(previous_point, current_point, left_w, right_w, left_u, right_u, m_context.m_vertices);
                }
                else {
                    m_context.m_vertices.emplace_back(Vector2f(current_point.pos.x + current_point.dm.x * left_w,
                                                               current_point.pos.y + current_point.dm.y * left_w),
                                                      Vector2f(left_u, 1));
                    m_context.m_vertices.emplace_back(Vector2f(current_point.pos.x - current_point.dm.x * right_w,
                                                               current_point.pos.y - current_point.dm.y * right_w),
                                                      Vector2f(right_u, 1));
                }
            }

            // copy the first two vertices from the beginning to form a cohesive loop
            assert(m_context.m_vertices.size() >= static_cast<size_t>(render_path.stroke_offset + 2));
            m_context.m_vertices.emplace_back(m_context.m_vertices[static_cast<size_t>(render_path.stroke_offset + 0)].pos,
                                              Vector2f(left_u, 1));
            m_context.m_vertices.emplace_back(m_context.m_vertices[static_cast<size_t>(render_path.stroke_offset + 1)].pos,
                                              Vector2f(right_u, 1));

            render_path.stroke_count = static_cast<GLsizei>(m_context.m_vertices.size() - static_cast<size_t>(render_path.stroke_offset));
        }
    }

    // create the polygon onto which to render the shape
    assert(m_context.m_vertices.size() < std::numeric_limits<GLint>::max());
    render_call.polygon_offset = static_cast<GLint>(m_context.m_vertices.size());
    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.left(), m_bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.right(), m_bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.right(), m_bounds.top()}, Vector2f{.5f, 1.f}});

    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.left(), m_bounds.bottom()}, Vector2f{.5f, 1.f}});
    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.right(), m_bounds.top()}, Vector2f{.5f, 1.f}});
    m_context.m_vertices.emplace_back(Vertex{Vector2f{m_bounds.left(), m_bounds.top()}, Vector2f{.5f, 1.f}});

    // create the shader uniforms for the call
    render_call.uniform_offset = static_cast<GLintptr>(m_context.m_shader_variables.size()) * m_context.fragmentSize();

    if (render_call.type == RenderContext::Call::Type::FILL) {
        // create an additional uniform buffer for a simple shader for the stencil
        RenderContext::ShaderVariables& stencil_uniforms = create_back(m_context.m_shader_variables);
        stencil_uniforms.strokeThr                       = -1;
        stencil_uniforms.type                            = RenderContext::ShaderVariables::Type::SIMPLE;
    }

    RenderContext::ShaderVariables& fill_uniforms = create_back(m_context.m_shader_variables);
    paint_to_frag(fill_uniforms, fill_paint, state.scissor, fringe_width, fringe_width, -1.0f);
}

void Painterpreter::_stroke()
{
    const PainterState& state = _get_current_state();

    // get the stroke paint
    Paint stroke_paint = state.stroke_paint;
    stroke_paint.inner_color.a *= state.alpha;
    stroke_paint.outer_color.a *= state.alpha;

    const float fringe_width = m_context.get_fringe_width();
    float stroke_width;
    { // create a sane stroke width
        const float scale = (state.xform.scale_factor_x() + state.xform.scale_factor_y()) / 2;
        stroke_width      = clamp(state.stroke_width * scale, 0, 200); // 200 is arbitrary
        if (stroke_width < fringe_width) {
            // if the stroke width is less than pixel size, use alpha to emulate coverage.
            const float alpha = clamp(stroke_width / fringe_width, 0.0f, 1.0f);
            stroke_paint.inner_color.a *= alpha * alpha; // since coverage is area, scale by alpha*alpha
            stroke_paint.outer_color.a *= alpha * alpha;
            stroke_width = fringe_width;
        }
        //
        if (m_context.provides_geometric_aa()) {
            stroke_width = (stroke_width / 2.f) + (fringe_width / 2.f);
        }
        else {
            stroke_width /= 2.f;
        }
    }

    // create the Cell's render call
    RenderContext::Call& render_call = create_back(m_context.m_calls);
    render_call.type                 = RenderContext::Call::Type::STROKE;
    render_call.path_offset          = m_context.m_paths.size();
    render_call.texture              = state.stroke_paint.texture;
    render_call.polygon_offset       = 0;

    size_t cap_divisions; // TODO: can circles get more coarse the further away they are?
    {                     // calculate divisions per half circle
        float da      = acos(stroke_width / (stroke_width + m_context.get_tesselation_tolerance())) * 2;
        cap_divisions = max(size_t(2), static_cast<size_t>(ceilf(static_cast<float>(PI) / da)));
    }

    const Painter::LineJoin line_join = state.line_join;
    const Painter::LineCap line_cap   = state.line_cap;
    _prepare_paths(stroke_width, line_join, state.miter_limit);

    for (Path& path : m_paths) {
        assert(path.point_count > 1);

        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < m_points.size());

        RenderContext::Path& render_path = create_back(m_context.m_paths);
        assert(m_context.m_vertices.size() < std::numeric_limits<GLint>::max());
        render_path.stroke_offset = static_cast<GLint>(m_context.m_vertices.size());

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
                _create_butt_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, fringe_width / -2, fringe_width, m_context.m_vertices);
                break;
            case Painter::LineCap::SQUARE:
                _create_butt_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, stroke_width - fringe_width, fringe_width, m_context.m_vertices);
                break;
            case Painter::LineCap::ROUND:
                _create_round_cap_start(m_points[previous_offset], m_points[previous_offset].forward, stroke_width, cap_divisions, m_context.m_vertices);
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
                    _create_round_join(previous_point, current_point, stroke_width, cap_divisions, m_context.m_vertices);
                }
                else {
                    _create_bevel_join(previous_point, current_point, stroke_width, stroke_width, 0, 1, m_context.m_vertices);
                }
            }
            else {
                m_context.m_vertices.emplace_back(current_point.pos + (current_point.dm * stroke_width),
                                                  Vector2f(0, 1));
                m_context.m_vertices.emplace_back(current_point.pos - (current_point.dm * stroke_width),
                                                  Vector2f(1, 1));
            }
        }

        if (path.is_closed) {
            // loop it
            assert(m_context.m_vertices.size() >= static_cast<size_t>(render_path.stroke_offset + 2));
            m_context.m_vertices.emplace_back(m_context.m_vertices[static_cast<size_t>(render_path.stroke_offset + 0)].pos,
                                              Vector2f(0, 1));
            m_context.m_vertices.emplace_back(m_context.m_vertices[static_cast<size_t>(render_path.stroke_offset + 1)].pos,
                                              Vector2f(1, 1));
        }
        else {
            // add cap
            switch (line_cap) {
            case Painter::LineCap::BUTT:
                _create_butt_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, fringe_width / -2, fringe_width, m_context.m_vertices);
                break;
            case Painter::LineCap::SQUARE:
                _create_butt_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, stroke_width - fringe_width, fringe_width, m_context.m_vertices);
                break;
            case Painter::LineCap::ROUND:
                _create_round_cap_end(m_points[current_offset], m_points[previous_offset].forward, stroke_width, cap_divisions, m_context.m_vertices);
                break;
            default:
                assert(0);
            }
        }
        render_path.stroke_count = static_cast<GLsizei>(m_context.m_vertices.size() - static_cast<size_t>(render_path.stroke_offset));
    }
    render_call.path_count = m_context.m_paths.size() - render_call.path_offset;

    // create the shader uniforms for the call
    render_call.uniform_offset                       = static_cast<GLintptr>(m_context.m_shader_variables.size()) * m_context.fragmentSize();
    RenderContext::ShaderVariables& stencil_uniforms = create_back(m_context.m_shader_variables);
    paint_to_frag(stencil_uniforms, stroke_paint, state.scissor, stroke_width, fringe_width, -1.0f);

    // I don't know what the stroke_threshold below is, but with -1 you get artefacts in the rotating lines test
    RenderContext::ShaderVariables& stroke_uniforms = create_back(m_context.m_shader_variables);
    paint_to_frag(stroke_uniforms, stroke_paint, state.scissor, stroke_width, fringe_width, 1.0f - 0.5f / 255.0f);
}

void Painterpreter::_prepare_paths(const float fringe, const Painter::LineJoin join, const float miter_limit)
{
    for (size_t path_index = 0; path_index < m_paths.size(); ++path_index) {
        Path& path = m_paths[path_index];

        // if the first and last points are the same, remove the last one and mark the path as closed
        if (path.point_count >= 2) {
            const Point& first = m_points[path.first_point];
            const Point& last  = m_points[path.first_point + path.point_count - 1];
            if (first.pos.is_approx(last.pos, m_context.get_distance_tolerance())) {
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
        const float area = _get_path_area(path);
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

            // update the bounds
            m_bounds._min.x = min(m_bounds._min.x, current_point.pos.x);
            m_bounds._min.y = min(m_bounds._min.y, current_point.pos.y);
            m_bounds._max.x = max(m_bounds._max.x, current_point.pos.x);
            m_bounds._max.y = max(m_bounds._max.y, current_point.pos.y);
        }

        path.is_convex = (left_turn_count == path.point_count);
    }
}

void paint_to_frag(RenderContext::ShaderVariables& frag, const Paint& paint, const Scissor& scissor,
                   const float stroke_width, const float fringe, const float stroke_threshold)
{
    assert(fringe > 0);

    // TODO: I don't know if scissor is ever anything else than identiy & -1/-1

    frag.innerCol = paint.inner_color.premultiplied();
    frag.outerCol = paint.outer_color.premultiplied();
    if (scissor.extend.width < 1.0f || scissor.extend.height < 1.0f) { // extend cannot be less than a pixel
        frag.scissorExt[0]   = 1.0f;
        frag.scissorExt[1]   = 1.0f;
        frag.scissorScale[0] = 1.0f;
        frag.scissorScale[1] = 1.0f;
    }
    else {
        xformToMat3x4(frag.scissorMat, scissor.xform.get_inverse());
        frag.scissorExt[0]   = scissor.extend.width / 2;
        frag.scissorExt[1]   = scissor.extend.height / 2;
        frag.scissorScale[0] = sqrt(scissor.xform[0][0] * scissor.xform[0][0] + scissor.xform[1][0] * scissor.xform[1][0]) / fringe;
        frag.scissorScale[1] = sqrt(scissor.xform[0][1] * scissor.xform[0][1] + scissor.xform[1][1] * scissor.xform[1][1]) / fringe;
    }
    frag.extent[0]  = paint.extent.width;
    frag.extent[1]  = paint.extent.height;
    frag.strokeMult = (stroke_width * 0.5f + fringe * 0.5f) / fringe;
    frag.strokeThr  = stroke_threshold;

    if (paint.texture) {
        frag.type    = RenderContext::ShaderVariables::Type::IMAGE;
        frag.texType = paint.texture->get_format() == Texture2::Format::GRAYSCALE ? 2 : 0; // TODO: change the 'texType' uniform in the shader to something more meaningful
    }
    else { // no image
        frag.type    = RenderContext::ShaderVariables::Type::GRADIENT;
        frag.radius  = paint.radius;
        frag.feather = paint.feather;
    }
    xformToMat3x4(frag.paintMat, paint.xform.get_inverse());
}

} // namespace notf