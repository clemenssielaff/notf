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

#include "graphics/painter.hpp"

#include "common/line2.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"
#include "graphics/cell.hpp"
#include "graphics/render_context_old.hpp"
#include "graphics/vertex.hpp"

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

    /** Miter distance. */
    Vector2f dm;

    /** Distance to the next point forward. */
    float length;

    /** Additional information about this Point. */
    Flags flags;
};

/**********************************************************************************************************************/

struct PainterPath {

    PainterPath(std::vector<PainterPoint>& points)
        : first_point(points.size())
        , point_count(0)
        , winding(Painter::Winding::COUNTERCLOCKWISE)
        , is_closed(false)
        , is_convex(true)
    {
    }

    /** Index of the first Point. */
    size_t first_point;

    /** Number of Points in this Path. */
    size_t point_count;

    /** What direction the Path is wound. */
    Painter::Winding winding;

    /** Whether this Path is closed or not.
     * Closed Paths will draw an additional line between their last and first Point.
     */
    bool is_closed;

    /** Whether the Path is convex or not. */
    bool is_convex;
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
        paths.back().point_count++;
    }

    /** Creates a new, empty Path. */
    void add_path() { paths.emplace_back(points); }

    /** Calculates the area of a given Path. */
    float get_path_area(const PainterPath& path)
    {
        float area            = 0;
        const PainterPoint& a = points[0];
        for (size_t index = path.first_point + 2; index < path.first_point + path.point_count; ++index) {
            const PainterPoint& b = points[index - 1];
            const PainterPoint& c = points[index];
            area += (c.pos.x - a.pos.x) * (b.pos.y - a.pos.y) - (b.pos.x - a.pos.x) * (c.pos.y - a.pos.y);
        }
        return area / 2;
    }

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

/** Chooses whether to bevel a joint or not and returns vertex coordinates. */
std::tuple<float, float, float, float>
choose_bevel(bool is_beveling, const PainterPoint& prev_point, const PainterPoint& curr_point, const float stroke_width)
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

void create_bevel_join(const PainterPoint& previous_point, const PainterPoint& current_point, const float left_w, const float right_w,
                       const float left_u, const float right_u, std::vector<Vertex>& vertices_out)
{
    if (current_point.flags & PainterPoint::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(current_point.flags & PainterPoint::Flags::INNERBEVEL,
                                                    previous_point, current_point, left_w);

        vertices_out.emplace_back(Vector2f(lx0, ly0),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(current_point.pos.x - previous_point.forward.y * right_w,
                                           current_point.pos.y + previous_point.forward.x * right_w),
                                  Vector2f(right_u, 1));

        if (current_point.flags & PainterPoint::Flags::BEVEL) {
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
        std::tie(rx0, ry0, rx1, ry1) = choose_bevel(current_point.flags & PainterPoint::Flags::INNERBEVEL,
                                                    previous_point, current_point, -right_w);

        vertices_out.emplace_back(Vector2f(current_point.pos.x + previous_point.forward.y * left_w,
                                           current_point.pos.y - previous_point.forward.x * left_w),
                                  Vector2f(left_u, 1));
        vertices_out.emplace_back(Vector2f(rx0, ry0),
                                  Vector2f(right_u, 1));

        if (current_point.flags & PainterPoint::Flags::BEVEL) {
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

void create_round_join(const PainterPoint& previous_point, const PainterPoint& current_point, const float stroke_width,
                       const size_t divisions, std::vector<Vertex>& vertices_out)
{
    if (current_point.flags & PainterPoint::Flags::LEFT) {
        float lx0, ly0, lx1, ly1;
        std::tie(lx0, ly0, lx1, ly1) = choose_bevel(
            current_point.flags & PainterPoint::Flags::INNERBEVEL, previous_point, current_point, stroke_width);
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
            current_point.flags & PainterPoint::Flags::INNERBEVEL, previous_point, current_point, -stroke_width);
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

/** Creates the round cap and the start of a line.
 * @param point         Start point of the line.
 * @param delta         Direction of the cap.
 * @param stroke_width  Width of the base of the cap.
 * @param divisions     Number of divisions for the cap (per half-circle).
 * @param vertices_out  [out] Vertex vector to write the cap into.
 */
void create_round_cap_start(const PainterPoint& point, const Vector2f& delta, const float stroke_width,
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

/** Creates the round cap and the end of a line.
 * @param point         End point of the line.
 * @param delta         Direction of the cap.
 * @param stroke_width  Width of the base of the cap.
 * @param divisions     Number of divisions for the cap (per half-circle).
 * @param vertices_out  [out] Vertex vector to write the cap into.
 */
void create_round_cap_end(const PainterPoint& point, const Vector2f& delta, const float stroke_width,
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

void create_butt_cap_start(const PainterPoint& point, const Vector2f& direction, const float stroke_width,
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

void create_butt_cap_end(const PainterPoint& point, const Vector2f& delta, const float stroke_width,
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

/**********************************************************************************************************************/

/** Length of a bezier control vector to draw a circle with radius 1. */
static const float KAPPAf = static_cast<float>(KAPPA);

} // namespace anonymous

namespace notf {

/**********************************************************************************************************************/

std::vector<Painter::State> Painter::s_states;
std::vector<size_t> Painter::s_state_succession;

Painter::Painter(Cell& cell, const RenderContext_Old& context)
    : m_cell(cell)
    , m_context(context)
    , m_stylus(Vector2f::zero())
{
    // states
    s_states.clear();
    s_states.emplace_back();
    s_state_succession.clear();
    s_state_succession.emplace_back(0);

    // data
    const float pixel_ratio = m_context.get_pixel_ratio();
    assert(abs(pixel_ratio) > 0);
    g_data.clear();
    g_data.distance_tolerance    = m_context.get_distance_tolerance();
    g_data.tesselation_tolerance = m_context.get_tesselation_tolerance();
    g_data.fringe_width          = m_context.get_fringe_width();
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
    s_state_succession.push_back(s_states.size() - 1);
    _append_commands({to_float(Command::NEXT_STATE)});
    return stack_height;
}

size_t Painter::pop_state()
{
    size_t stack_height = 1;
    if (s_state_succession.size() == 1) {
        return stack_height;
    }
    s_state_succession.push_back(_get_current_state().previous_state);
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

void Painter::set_fill_paint(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.fill = std::move(paint);
}

void Painter::set_stroke_paint(Paint paint)
{
    State& current_state = _get_current_state();
    paint.xform *= current_state.xform;
    current_state.stroke = std::move(paint);
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

void Painter::_fill()
{
    const State& state = _get_current_state();

    // get the fill paint
    Paint fill_paint = state.fill;
    fill_paint.inner_color.a *= state.alpha;
    fill_paint.outer_color.a *= state.alpha;

    const float fringe = m_context.provides_geometric_aa() ? g_data.fringe_width : 0;
    _prepare_paths(fringe, LineJoin::MITER, 2.4f);

    // create the Cell's render call
    Cell::Call& render_call = create_back(m_cell.m_calls);
    if (g_data.paths.size() == 1 && g_data.paths.front().is_convex) {
        render_call.type = Cell::Call::Type::CONVEX_FILL;
    }
    else {
        render_call.type = Cell::Call::Type::FILL;
    }
    render_call.path_offset = m_cell.m_paths.size();
    render_call.paint       = fill_paint;

    const float woff = fringe / 2;
    for (PainterPath& path : g_data.paths) {
        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < g_data.points.size());

        // create the fill vertices
        Cell::Path& cell_path = create_back(m_cell.m_paths);
        cell_path.fill_offset = m_cell.m_vertices.size();
        if (fringe > 0) {
            // create a loop
            for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const PainterPoint& previous_point = g_data.points[previous_offset];
                const PainterPoint& current_point  = g_data.points[current_offset];

                // no bevel
                if (!(current_point.flags & PainterPoint::Flags::BEVEL) || current_point.flags & PainterPoint::Flags::LEFT) {
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
            for (size_t point_offset = path.first_point; point_offset <= last_point_offset; ++point_offset) {
                m_cell.m_vertices.emplace_back(g_data.points[point_offset].pos,
                                               Vector2f(.5f, 1));
            }
        }
        cell_path.fill_count = m_cell.m_vertices.size() - cell_path.fill_offset;

        // create stroke vertices, if we draw this shape antialiased
        if (fringe > 0) {
            cell_path.stroke_offset = m_cell.m_vertices.size();

            float left_w        = fringe + woff;
            float left_u        = 0;
            const float right_w = fringe - woff;
            const float right_u = 1;

            { // create only half a fringe for convex shapes so that the shape can be rendered without stenciling
                const bool is_convex = g_data.paths.size() == 1 && g_data.paths.front().is_convex;
                if (is_convex) {
                    left_w = woff; // this should generate the same vertex as fill inset above
                    left_u = 0.5f; // set outline fade at middle
                }
            }

            // create a loop
            for (size_t current_offset = path.first_point, previous_offset = last_point_offset;
                 current_offset <= last_point_offset;
                 previous_offset = current_offset++) {
                const PainterPoint& previous_point = g_data.points[previous_offset];
                const PainterPoint& current_point  = g_data.points[current_offset];

                if (current_point.flags & (PainterPoint::Flags::BEVEL | PainterPoint::Flags::INNERBEVEL)) {
                    create_bevel_join(previous_point, current_point, left_w, right_w, left_u, right_u, m_cell.m_vertices);
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
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[cell_path.stroke_offset + 0].pos,
                                           Vector2f(left_u, 1));
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[cell_path.stroke_offset + 1].pos,
                                           Vector2f(right_u, 1));

            cell_path.stroke_count = m_cell.m_vertices.size() - cell_path.stroke_offset;
        }
    }
    render_call.path_count = m_cell.m_paths.size() - render_call.path_offset;
}

void Painter::_stroke()
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
        if (stroke_width < g_data.fringe_width) {
            // if the stroke width is less than pixel size, use alpha to emulate coverage.
            const float alpha = clamp(stroke_width / g_data.fringe_width, 0.0f, 1.0f);
            stroke_paint.inner_color.a *= alpha * alpha; // since coverage is area, scale by alpha*alpha
            stroke_paint.outer_color.a *= alpha * alpha;
            stroke_width = g_data.fringe_width;
        }
        //
        if (m_context.provides_geometric_aa()) {
            stroke_width = (stroke_width / 2.f) + (g_data.fringe_width / 2.f);
        }
        else {
            stroke_width /= 2.f;
        }
    }

    // create the Cell's render call
    Cell::Call& render_call = create_back(m_cell.m_calls);
    render_call.type        = Cell::Call::Type::STROKE;
    render_call.path_offset = m_cell.m_paths.size();
    render_call.paint       = stroke_paint;
    render_call.scissor     = _get_current_state().scissor;

    size_t cap_divisions;
    { // calculate divisions per half circle
        float da      = acos(stroke_width / (stroke_width + g_data.tesselation_tolerance)) * 2;
        cap_divisions = max(size_t(2), static_cast<size_t>(ceilf(static_cast<float>(PI) / da)));
    }

    const LineJoin line_join = _get_current_state().line_join;
    const LineCap line_cap   = _get_current_state().line_cap;
    _prepare_paths(stroke_width, line_join, _get_current_state().miter_limit);

    for (PainterPath& path : g_data.paths) {
        assert(path.point_count > 1);

        const size_t last_point_offset = path.first_point + path.point_count - 1;
        assert(last_point_offset < g_data.points.size());

        Cell::Path& cell_path   = create_back(m_cell.m_paths);
        cell_path.stroke_offset = m_cell.m_vertices.size();

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
            case LineCap::BUTT:
                create_butt_cap_start(g_data.points[previous_offset], g_data.points[previous_offset].forward, stroke_width, g_data.fringe_width / -2, g_data.fringe_width, m_cell.m_vertices);
                break;
            case LineCap::SQUARE:
                create_butt_cap_start(g_data.points[previous_offset], g_data.points[previous_offset].forward, stroke_width, stroke_width - g_data.fringe_width, g_data.fringe_width, m_cell.m_vertices);
                break;
            case LineCap::ROUND:
                create_round_cap_start(g_data.points[previous_offset], g_data.points[previous_offset].forward, stroke_width, cap_divisions, m_cell.m_vertices);
                break;
            default:
                assert(0);
            }
        }

        for (; current_offset <= last_point_offset - (path.is_closed ? 0 : 1);
             previous_offset = current_offset++) {
            const PainterPoint& previous_point = g_data.points[previous_offset];
            const PainterPoint& current_point  = g_data.points[current_offset];

            if (current_point.flags & (PainterPoint::Flags::BEVEL | PainterPoint::Flags::INNERBEVEL)) {
                if (line_join == LineJoin::ROUND) {
                    create_round_join(previous_point, current_point, stroke_width, cap_divisions, m_cell.m_vertices);
                }
                else {
                    create_bevel_join(previous_point, current_point, stroke_width, stroke_width, 0, 1, m_cell.m_vertices);
                }
            }
            else {
                m_cell.m_vertices.emplace_back(current_point.pos + (current_point.dm * stroke_width),
                                               Vector2f(0, 1));
                m_cell.m_vertices.emplace_back(current_point.pos - (current_point.dm * stroke_width),
                                               Vector2f(1, 1));
            }
        }

        if (path.is_closed) {
            // loop it
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[cell_path.stroke_offset + 0].pos, Vector2f(0, 1));
            m_cell.m_vertices.emplace_back(m_cell.m_vertices[cell_path.stroke_offset + 1].pos, Vector2f(1, 1));
        }
        else {
            // add cap
            switch (line_cap) {
            case LineCap::BUTT:
                create_butt_cap_end(g_data.points[current_offset], g_data.points[previous_offset].forward, stroke_width, g_data.fringe_width / -2, g_data.fringe_width, m_cell.m_vertices);
                break;
            case LineCap::SQUARE:
                create_butt_cap_end(g_data.points[current_offset], g_data.points[previous_offset].forward, stroke_width, stroke_width - g_data.fringe_width, g_data.fringe_width, m_cell.m_vertices);
                break;
            case LineCap::ROUND:
                create_round_cap_end(g_data.points[current_offset], g_data.points[previous_offset].forward, stroke_width, cap_divisions, m_cell.m_vertices);
                break;
            default:
                assert(0);
            }
        }
        cell_path.stroke_count = m_cell.m_vertices.size() - cell_path.stroke_offset;
    }
    render_call.path_count = m_cell.m_paths.size() - render_call.path_offset;
}

void Painter::_prepare_paths(const float fringe, const LineJoin join, const float miter_limit)
{
    for (size_t path_index = 0; path_index < g_data.paths.size(); ++path_index) {
        PainterPath& path = g_data.paths[path_index];

        // if the first and last points are the same, remove the last one and mark the path as closed
        if (path.point_count >= 2) {
            const PainterPoint& first = g_data.points[path.first_point];
            const PainterPoint& last  = g_data.points[path.first_point + path.point_count - 1];
            if (first.pos.is_approx(last.pos, g_data.distance_tolerance)) {
                --path.point_count;
                path.is_closed = true;
            }
        }

        // remove paths with just a single point
        if (path.point_count < 2) {
            g_data.paths.erase(iterator_at(g_data.paths, path_index));
            --path_index;
            continue;
        }

        // enforce winding
        const float area = g_data.get_path_area(path);
        if ((path.winding == Winding::CCW && area < 0)
            || (path.winding == Winding::CW && area > 0)) {
            for (size_t i = path.first_point, j = path.first_point + path.point_count - 1; i < j; ++i, --j) {
                std::swap(g_data.points[i], g_data.points[j]);
            }
        }

        // make a first pass over all points in the path to determine their `forward` and `length` fields
        const size_t last_point_offset = path.first_point + path.point_count - 1;
        for (size_t next_offset = path.first_point, current_offset = last_point_offset;
             next_offset <= last_point_offset;
             current_offset = next_offset++) {
            PainterPoint& current_point = g_data.points[current_offset];
            PainterPoint& next_point    = g_data.points[next_offset];

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
            const PainterPoint& previous_point = g_data.points[previous_offset];
            PainterPoint& current_point        = g_data.points[current_offset];

            // clear all flags except the corner (in case the Path has already been prepared one)
            current_point.flags = (current_point.flags & PainterPoint::Flags::CORNER) ? PainterPoint::Flags::CORNER
                                                                                      : PainterPoint::Flags::NONE;

            // keep track of left turns
            if (current_point.forward.cross(previous_point.forward) > 0) {
                ++left_turn_count;
                current_point.flags = static_cast<PainterPoint::Flags>(current_point.flags | PainterPoint::Flags::LEFT);
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
                current_point.flags = static_cast<PainterPoint::Flags>(current_point.flags | PainterPoint::Flags::INNERBEVEL);
            }

            // check to see if the corner needs to be beveled
            if (current_point.flags & PainterPoint::Flags::CORNER) {
                if (join == LineJoin::BEVEL || join == LineJoin::ROUND || (dm_mag_sq * miter_limit * miter_limit) < 1.0f) {
                    current_point.flags = static_cast<PainterPoint::Flags>(current_point.flags | PainterPoint::Flags::BEVEL);
                }
            }

            // update the Cell's bounds
            m_cell.m_bounds._min.x = min(m_cell.m_bounds._min.x, current_point.pos.x);
            m_cell.m_bounds._min.y = min(m_cell.m_bounds._min.y, current_point.pos.y);
            m_cell.m_bounds._max.x = max(m_cell.m_bounds._max.x, current_point.pos.x);
            m_cell.m_bounds._max.y = max(m_cell.m_bounds._max.y, current_point.pos.y);
        }

        path.is_convex = (left_turn_count == path.point_count);
    }
}

// TODO: when this draws, revisit this function and see if you find a way to guarantee that there is always a path and point without if-statements all over the place
void Painter::_execute()
{
    assert(!s_state_succession.empty());
    std::vector<size_t> state_succession = s_state_succession;
    s_state_succession.clear();
    size_t state_index = 0;
    s_state_succession.push_back(state_succession[state_index]);

    // prepare the cell
    m_cell.m_bounds = Aabrf::wrongest();

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
            if (++state_index < state_succession.size()) {
                s_state_succession.push_back(state_succession[state_index]);
            }
            else {
                log_critical << "State succession missmatch detected during execution of the Command buffer";
            }
            break;

        case Command::FILL:
            _fill();
            break;

        case Command::STROKE:
            _stroke();
            break;

        default: // unknown enum
            assert(0);
        }
    }

    // finish the Cell
    if (!m_cell.m_bounds.is_valid()) {
        m_cell.m_bounds = Aabrf::null();
    }
}

} // namespace notf
