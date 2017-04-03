#pragma once

#include <vector>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/float.hpp"
#include "common/size2.hpp"
#include "common/xform2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/texture2.hpp"
#include "graphics/vertex.hpp"
#include "utils/enum_to_number.hpp"

#include "graphics/painter.hpp"

namespace notf {

class RenderContext_Old;

/**********************************************************************************************************************/

struct Scissor_Old {
    /** Scissors have their own transformation. */
    Xform2f xform;

    /** Extend around the center of the Transform. */
    Size2f extend;
};

/**********************************************************************************************************************/

enum class LineCap : unsigned char {
    BUTT,
    ROUND,
    SQUARE,
};

/**********************************************************************************************************************/

enum class LineJoin : unsigned char {
    MITER,
    ROUND,
    BEVEL,
};

/**********************************************************************************************************************/

enum class Winding : unsigned char {
    CCW,
    CW,
    COUNTERCLOCKWISE = CCW,
    CLOCKWISE        = CW,
    SOLID            = CCW,
    HOLE             = CW,
};

/**********************************************************************************************************************/

namespace detail {

/** A Cell path is a slice of Cell::m_vertices.
 * It is in the `detail` namespace, because it is used both by Cell and RenderContext.
 */
struct CellPath {
    size_t point_offset; // points
    size_t point_count;
    bool is_closed;
    size_t fill_offset; // vertices
    size_t fill_count;
    size_t stroke_offset; // vertices
    size_t stroke_count;
    Winding winding;
    bool is_convex;

    CellPath(size_t first)
        : point_offset(first)
        , point_count(0)
        , is_closed(false)
        , fill_offset(0)
        , fill_count(0)
        , stroke_offset(0)
        , stroke_count(0)
        , winding(Winding::COUNTERCLOCKWISE)
        , is_convex(false)
    {
    }
};

} // namespace detail

/**********************************************************************************************************************/

/** Each Widget draws itself into a 'Cell'.
 * We can use the Cell to move the widget on the Canvas without redrawing it, much like you could re-use a 'Cel' in
 * traditional animation ( see: https://en.wikipedia.org/wiki/Traditional_animation ).
 * Technically the name should be 'Cel' with a single 'l', but 'Cell' works as well and is more easily understood.
 */
class Cell_Old {

private: // class
    /** Command identifyers, type must be of the same size as a float. */
    enum class Command : uint32_t {
        MOVE = 0,
        LINE,
        BEZIER,
        WINDING,
        CLOSE,
    };
    static_assert(sizeof(Cell_Old::Command) == sizeof(float),
                  "Floats on your system don't seem be to be 32 bits wide. "
                  "Adjust the type of the underlying type of CommandBuffer::Command to fit your particular system.");

    /******************************************************************************************************************/

    struct Point {
        enum Flags : unsigned char {
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

    using Path = detail::CellPath;

    /******************************************************************************************************************/

    struct RenderState {
        RenderState()
            : stroke_width(1)
            , miter_limit(10)
            , alpha(1)
            , xform(Xform2f::identity())
            , blend_mode(BlendMode::SOURCE_OVER)
            , line_cap(LineCap::BUTT)
            , line_join(LineJoin::MITER)
            , fill(Color(255, 255, 255))
            , stroke(Color(0, 0, 0))
            , scissor({Xform2f::identity(), {-1, -1}})
        {
        }

        RenderState(const RenderState& other) = default;

        float stroke_width;
        float miter_limit;
        float alpha;
        Xform2f xform;
        BlendMode blend_mode;
        LineCap line_cap;
        LineJoin line_join;
        Paint fill;
        Paint stroke;
        Scissor_Old scissor;
    };

public: // methods
    Cell_Old();

    void reset(const RenderContext_Old& context);

    size_t push_state();

    size_t pop_state();

    const RenderState& get_current_state() const;

    void set_stroke_width(const float width) { _get_current_state().stroke_width = width; }

    void set_miter_limit(const float limit) { _get_current_state().miter_limit = limit; }

    void set_line_cap(const LineCap cap) { _get_current_state().line_cap = cap; }

    void set_line_join(const LineJoin join) { _get_current_state().line_join = join; }

    void set_alpha(const float alpha) { _get_current_state().alpha = alpha; }

    void set_stroke_color(const Color color) { _get_current_state().stroke.set_color(std::move(color)); }

    void set_stroke_paint(Paint paint);

    void set_fill_color(const Color color) { _get_current_state().fill.set_color(std::move(color)); }

    void set_fill_paint(Paint paint);

    void set_blend_mode(const BlendMode mode) { _get_current_state().blend_mode = std::move(mode); }

    void translate(const float x, const float y) { translate(Vector2f{x, y}); }

    void translate(const Vector2f delta) { _get_current_state().xform *= Xform2f::translation(std::move(delta)); }

    /** Rotates the current state the given amount of radians in a counter-clockwise direction. */
    void rotate(const float angle) { _get_current_state().xform = Xform2f::rotation(angle) * _get_current_state().xform; }

    void transform(const Xform2f& transform) { _get_current_state().xform *= transform; }

    void reset_transform() { _get_current_state().xform = Xform2f::identity(); }

    const Xform2f& get_transform() const { return get_current_state().xform; }

    void set_scissor(const Aabrf& aabr);

    void reset_scissor() { _get_current_state().scissor = {Xform2f::identity(), {-1, -1}}; }

    // nanovg has a function `nvgIntersectScissor` that tries to approximate the intersection of two (not aligned)
    // rectangles ... I leave that out for now




    //

    void begin_path();

    void move_to(const Vector2f pos) { move_to(pos.x, pos.y); }

    void move_to(const float x, const float y);

    void line_to(const Vector2f pos) { line_to(pos.x, pos.y); }

    void line_to(const float x, const float y);

    void bezier_to(const Vector2f ctrl1, const Vector2f ctrl2, const Vector2f end)
    {
        bezier_to(ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, end.x, end.y);
    }

    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty);

    void add_rect(const Aabrf& rect) { add_rect(rect.left(), rect.top(), rect.width(), rect.height()); }

    void add_rect(const float x, const float y, const float w, const float h);

    void add_ellipse(const Vector2f center, const Size2f extend)
    {
        add_ellipse(center.x, center.y, extend.width, extend.height);
    }

    void add_ellipse(const float cx, const float cy, const float rx, const float ry);

    void add_circle(const float cx, const float cy, const float radius)
    {
        add_ellipse(cx, cy, radius, radius);
    }

    void add_circle(const Vector2f center, const float radius)
    {
        add_ellipse(center.x, center.y, radius, radius);
    }

    void quad_to(const Vector2f ctrl, const Vector2f end)
    {
        quad_to(ctrl.x, ctrl.y, end.x, end.y);
    }

    void quad_to(const float cx, const float cy, const float tx, const float ty);

    void add_rounded_rect(const Aabrf& rect, const float radius)
    {
        add_rounded_rect(rect.left(), rect.top(), rect.width(), rect.height(), radius, radius, radius, radius);
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h, const float radius)
    {
        add_rounded_rect(x, y, w, h, radius, radius, radius, radius);
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h,
                          const float rtl, const float rtr, const float rbr, const float rbl);

    void arc_to(const Vector2f tangent, const Vector2f end, const float radius);

    /** Create an arc between two tangents on the canvas.
     * @param x1        X-coordinate of the first tangent.
     * @param y1        Y-coordinate of the first tangent.
     * @param x2        X-coordinate of the second tangent.
     * @param y2        Y-coordinate of the second tangent.
     * @param radius    Radius of the arc.
     * @see             http://www.w3schools.com/tags/canvas_arcto.asp
     */
    void arc_to(const float x1, const float y1, const float x2, const float y2, const float radius)
    {
        arc_to({x1, y1}, {x2, y2}, radius);
    }

    void arc(float cx, float cy, float r, float a0, float a1, Winding dir);

    void set_winding(const Winding winding);

    void close_path();

    void fill(RenderContext_Old& context);

    void stroke(RenderContext_Old& context);

public: // getter
    const std::vector<Path>& get_paths() const { return m_paths; }

    const std::vector<Vertex>& get_vertices() const { return m_vertices; }

    /** The bounding rectangle of the Cell.
     * Is independent of the Widget's AARB and used as quad onto which the Cell is rendered.
     */
    const Aabrf& get_bounds() const { return m_bounds; }

    float get_fringe_width() const { return m_fringe_width; }


private: // methods
    void _append_commands(std::vector<float>&& commands);

    RenderState& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

    void _flatten_paths();

    void _calculate_joins(const float fringe, const LineJoin join, const float miter_limit);

    void _expand_fill(const bool draw_antialiased);

    void _expand_stroke(const float stroke_width);

    /** Creates a new Point, but only if the position significantly differs from the last one. */
    void _add_point(const Vector2f position, const Point::Flags flags);

    void _tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                           const float x3, const float y3, const float x4, const float y4);

    void _butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width, const float d);

    void _butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width, const float d);

    void _round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width, const size_t cap_count);

    void _round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width, const size_t cap_count);

    void _bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                     const float left_u, const float right_u);

    void _round_join(const Point& previous_point, const Point& current_point, const float stroke_width, const size_t ncap);

private: // static methods
    float poly_area(const std::vector<Point>& points, const size_t offset, const size_t count);

    std::tuple<float, float, float, float>
    choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width);

    float to_float(const Command command);

private: // fields
    std::vector<RenderState> m_states;

    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<float> m_commands;

    /** Current position of the 'stylus', as the last Command left it. */
    Vector2f m_stylus;

    std::vector<Point> m_points;

    std::vector<Path> m_paths;

    std::vector<Vertex> m_vertices;

    /** The bounding rectangle of the Cell. */
    Aabrf m_bounds;

    float m_tesselation_tolerance;
    float m_distance_tolerance;
    float m_fringe_width;

};

} // namespace notf
