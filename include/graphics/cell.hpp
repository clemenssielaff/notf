#pragma once

#include <vector>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/float_utils.hpp"
#include "common/size2i.hpp"
#include "common/transform2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/vertex.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

class RenderContext;

struct Paint {
    Paint() = default;

    Paint(Color color)
        : xform(Transform2::identity())
        , extent()
        , radius(0)
        , feather(1)
        , innerColor(std::move(color))
        , outerColor(innerColor)
    {
    }

    void set_color(const Color color)
    {
        xform      = Transform2::identity();
        radius     = 0;
        feather    = 1;
        innerColor = std::move(color);
        outerColor = innerColor;
    }

    Transform2 xform;
    Size2f extent;
    float radius;
    float feather;
    Color innerColor;
    Color outerColor;
};

struct Scissor {
    /** Scissors have their own transformation. */
    Transform2 xform;

    /** Extend around the center of the Transform.
     * That means that the Scissor's width is extend.width * 2.
     */
    Size2f extend;
};

/** Each Widget draws itself into a 'Cell'.
 * We can use the Cell to move the widget on the Canvas without redrawing it, much like you could re-use a 'Cel' in
 * traditional animation ( see: https://en.wikipedia.org/wiki/Traditional_animation ).
 * Technically the name should be 'Cel' with a single 'l', but 'Cell' works as well and is more easily understood.
 */
class Cell {

public: // enum
    /** Command identifyers, type must be of the same size as a float. */
    enum class Command : uint32_t {
        MOVE = 0,
        LINE,
        BEZIER,
        WINDING,
        CLOSE,
        // NOOP,
        // FILL,
        // STROKE,
        // BEGIN,
    };

    enum class LineCap : unsigned char {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class LineJoin : unsigned char {
        MITER,
        ROUND,
        BEVEL,
    };

    enum class Winding : unsigned char {
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE        = CW,
        SOLID            = CCW,
        HOLE             = CW,
    };

    // The vertices of the Path are stored in Cell while this struct only stores offsets and sizes.
    struct Path {
        size_t point_offset; // points
        size_t point_count;
        bool is_closed;
        size_t bevel_count;
        size_t fill_offset; // vertices
        size_t fill_count;
        size_t stroke_offset; // vertices
        size_t stroke_count;
        Winding winding;
        bool is_convex;

        Path(size_t first)
            : point_offset(first)
            , point_count(0)
            , is_closed(false)
            , bevel_count(0)
            , fill_offset(0)
            , fill_count(0)
            , stroke_offset(0)
            , stroke_count(0)
            , winding(Winding::COUNTERCLOCKWISE)
            , is_convex(false)
        {
        }
    };

    struct Point {
        enum Flags : unsigned char {
            NONE       = 0,
            CORNER     = 1u << 1,
            LEFT       = 1u << 2,
            BEVEL      = 1u << 3,
            INNERBEVEL = 1u << 4,
        };

        /** Position of the Point. */
        Vector2 pos;

        /** Direction to the next Point. */
        Vector2 forward;

        /** Something with miter...? */
        Vector2 dm; // TODO: what is Point::dm?

        /** Distance to the next point forward. */
        float length;

        /** Additional information about this Point. */
        Flags flags;
    };

private: // class
    /******************************************************************************************************************/
    struct RenderState {
        RenderState()
            : stroke_width(1)
            , miter_limit(10)
            , alpha(1)
            , xform(Transform2::identity())
            , blend_mode(BlendMode::SOURCE_OVER)
            , line_cap(LineCap::BUTT)
            , line_join(LineJoin::MITER)
            , fill(Color(255, 255, 255))
            , stroke(Color(0, 0, 0))
            , scissor({Transform2::identity(), {-1, -1}})
        {
        }

        RenderState(const RenderState& other) = default;

        float stroke_width;
        float miter_limit;
        float alpha;
        Transform2 xform;
        BlendMode blend_mode;
        LineCap line_cap;
        LineJoin line_join;
        Paint fill;
        Paint stroke;
        Scissor scissor;
    };

public: // methods
    Cell();

    bool is_dirty() const { return m_is_dirty; }

    void set_dirty() { m_is_dirty = true; }

    void reset(const RenderContext& layer);

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

    void translate(const float x, const float y) { translate(Vector2{x, y}); }

    void translate(const Vector2 delta) { _get_current_state().xform *= Transform2::translation(std::move(delta)); }

    /** Rotates the current state the given amount of radians in a counter-clockwise direction. */
    void rotate(const float angle) { _get_current_state().xform = Transform2::rotation(angle) * _get_current_state().xform; }
    // TODO: Transform2::premultiply

    void transform(const Transform2& transform) { _get_current_state().xform *= transform; }

    void reset_transform() { _get_current_state().xform = Transform2::identity(); }

    const Transform2& get_transform() const { return get_current_state().xform; }

    void set_scissor(const Aabr& aabr);

    void reset_scissor() { _get_current_state().scissor = {Transform2::identity(), {-1, -1}}; }

    // nanovg has a function `nvgIntersectScissor` that tries to approximate the intersection of two (not aligned)
    // rectangles ... I leave that out for now

    void begin_path();

    void move_to(const Vector2 pos) { move_to(pos.x, pos.y); }

    void move_to(const float x, const float y);

    void line_to(const Vector2 pos) { line_to(pos.x, pos.y); }

    void line_to(const float x, const float y);

    void bezier_to(const Vector2 ctrl1, const Vector2 ctrl2, const Vector2 end)
    {
        bezier_to(ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, end.x, end.y);
    }

    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty);

    void add_rect(const Aabr& rect) { add_rect(rect.left(), rect.top(), rect.width(), rect.height()); }

    void add_rect(const float x, const float y, const float w, const float h);

    void add_ellipse(const Vector2 center, const Size2f extend)
    {
        add_ellipse(center.x, center.y, extend.width, extend.height);
    }

    void add_ellipse(const float cx, const float cy, const float rx, const float ry);

    void add_circle(const float cx, const float cy, const float radius)
    {
        add_ellipse(cx, cy, radius, radius);
    }

    void add_circle(const Vector2 center, const float radius)
    {
        add_ellipse(center.x, center.y, radius, radius);
    }

    void quad_to(const Vector2 ctrl, const Vector2 end)
    {
        quad_to(ctrl.x, ctrl.y, end.x, end.y);
    }

    void quad_to(const float cx, const float cy, const float tx, const float ty);

    void add_rounded_rect(const Aabr& rect, const float radius)
    {
        add_rounded_rect(rect.left(), rect.top(), rect.width(), rect.height(), radius, radius, radius, radius);
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h, const float radius)
    {
        add_rounded_rect(x, y, w, h, radius, radius, radius, radius);
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h,
                          const float rtl, const float rtr, const float rbr, const float rbl);

    void arc_to(const Vector2 tangent, const Vector2 end, const float radius);

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

    void fill(RenderContext& context);

    void stroke(RenderContext& context);

public: // getter
    const std::vector<Path>& get_paths() const { return m_paths; }

    const std::vector<Vertex>& get_vertices() const { return m_vertices; }

    const Aabr& get_bounds() const { return m_bounds; }

    float get_fringe_width() const { return m_fringe_width; }

public: // static methods
    static Paint create_linear_gradient(const Vector2& start_pos, const Vector2& end_pos,
                                        const Color start_color, const Color end_color);

    static Paint create_radial_gradient(const Vector2& center,
                                        const float inner_radius, const float outer_radius,
                                        const Color inner_color, const Color outer_color);

    static Paint create_box_gradient(const Vector2& center, const Size2f& extend,
                                     const float radius, const float feather,
                                     const Color inner_color, const Color outer_color);

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
    void _add_point(const Vector2 position, const Point::Flags flags);

    void _tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                           const float x3, const float y3, const float x4, const float y4);

    void _butt_cap_start(const Point& point, const Vector2& direction, const float stroke_width, const float d);

    void _butt_cap_end(const Point& point, const Vector2& delta, const float stroke_width, const float d);

    void _round_cap_start(const Point& point, const Vector2& delta, const float stroke_width, const size_t cap_count);

    void _round_cap_end(const Point& point, const Vector2& delta, const float stroke_width, const size_t cap_count);

    void _bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                     const float left_u, const float right_u);

    void _round_join(const Point& previous_point, const Point& current_point, const float stroke_width, const size_t ncap);

private: // fields
    std::vector<RenderState> m_states;

    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<float> m_commands;

    /** Index of the current Command. */
    size_t m_current_command;

    /** Current position of the 'stylus', as the last Command left it. */
    Vector2 m_stylus;

    std::vector<Point> m_points;

    std::vector<Path> m_paths;

    std::vector<Vertex> m_vertices;

    Aabr m_bounds; // TODO: Cell::m_bounds may be redundant when stored inside a Widget

    float m_tesselation_tolerance;
    float m_distance_tolerance;
    float m_fringe_width;

    bool m_is_dirty;
};

} // namespace notf
