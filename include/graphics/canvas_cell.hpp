#pragma once

#include <vector>

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/float_utils.hpp"
#include "common/size2i.hpp"
#include "common/transform2.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/vertex.hpp"
#include "render_backend.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

class CanvasLayer;

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
        SOLID = CCW,
        HOLES = CW,
    };

    struct Path { // belongs to Canvas
        int first;
        int count;
        unsigned char closed;
        int nbevel;
        std::vector<Vertex> fill;
        std::vector<Vertex> stroke;
        Winding winding;
        bool is_convex;
    };

    struct Point {
        enum Flag {
            CORNER     = 1u << 1,
            LEFT       = 1u << 2,
            BEVEL      = 1u << 3,
            INNERBEVEL = 1u << 4,
        };

        /** Position of the Point. */
        Vector2 pos;

        /** Vector to the next Point. */
        Vector2 delta;

        /** Something with miter...? */
        Vector2 dm; // TODO: what is Point::dm?

        /** Additional information about this Point. */
        Flag flags;
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

    void reset(const CanvasLayer& layer);

    size_t push_state();

    size_t pop_state();

    const RenderState& get_current_state() const;

    void set_stroke_width(const float width) { _get_current_state().stroke_width = std::move(width); }

    void set_miter_limit(const float limit) { _get_current_state().miter_limit = std::move(limit); }

    void set_line_cap(const LineCap cap) { _get_current_state().line_cap = std::move(cap); }

    void set_line_join(const LineJoin join) { _get_current_state().line_join = std::move(join); }

    void set_alpha(const float alpha) { _get_current_state().alpha = std::move(alpha); }

    void set_stroke_color(const Color color) { _get_current_state().stroke.set_color(std::move(color)); }

    void set_stroke_paint(Paint paint);

    void set_fill_color(const Color color) { _get_current_state().fill.set_color(std::move(color)); }

    void set_fill_paint(Paint paint);

    void set_blend_mode(const BlendMode mode) { _get_current_state().blend_mode = std::move(mode); }

    void transform(const Transform2& transform) { _get_current_state().xform *= transform; }

    void reset_transform() { _get_current_state().xform = Transform2::identity(); }

    const Transform2& get_transform() const { return get_current_state().xform; }

    void set_scissor(const Aabr& aabr);

    void reset_scissor() { _get_current_state().scissor = {Transform2::identity(), {-1, -1}}; }

    // nanovg has a function `nvgIntersectScissor` that tries to approximate the intersection of two (not aligned)
    // rectangles ... I leave that out for now

    void begin_path();

    void move_to(const Vector2 pos) { move_to(std::move(pos.x), std::move(pos.y)); }

    void move_to(const float x, const float y);

    void line_to(const Vector2 pos) { line_to(std::move(pos.x), std::move(pos.y)); }

    void line_to(const float x, const float y);

    void bezier_to(const Vector2 ctrl1, const Vector2 ctrl2, const Vector2 end)
    {
        bezier_to(std::move(ctrl1.x), std::move(ctrl1.y), std::move(ctrl2.x), std::move(ctrl2.y), std::move(end.x), std::move(end.y));
    }

    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty);

    void add_rect(const Aabr& rect) { add_rect(rect.x(), rect.y(), rect.width(), rect.height()); }

    void add_rect(const float x, const float y, const float w, const float h);

    void add_ellipse(const Vector2 center, const Size2f extend)
    {
        add_ellipse(std::move(center.x), std::move(center.y), std::move(extend.width), std::move(extend.height));
    }

    void add_ellipse(const float cx, const float cy, const float rx, const float ry);

    void add_circle(const float cx, const float cy, const float radius)
    {
        add_ellipse(std::move(cx), std::move(cy), radius, radius);
    }

    void add_circle(const Vector2 center, const float radius)
    {
        add_ellipse(std::move(center.x), std::move(center.y), radius, radius);
    }

    void quad_to(const Vector2 ctrl, const Vector2 end)
    {
        quad_to(std::move(ctrl.x), std::move(ctrl.y), std::move(end.x), std::move(end.y));
    }

    void quad_to(const float cx, const float cy, const float tx, const float ty);

    void add_rounded_rect(const Aabr& rect, const float radius)
    {
        add_rounded_rect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius, radius, radius);
    }

    void add_rounded_rect(const float x, const float y, const float w, const float h,
                          const float rtl, const float rtr, const float rbr, const float rbl);

    // nanovg has two additional methods: `nvgArc` and `nvgArcTo`, which are quite large so I ignore them for now

    void set_winding(const Winding winding);

    void close_path();

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

    Aabr m_bounds;

    float m_tesselation_tolerance;
    float m_distance_tolerance;
    float m_fringe_width;
};

} // namespace notf
