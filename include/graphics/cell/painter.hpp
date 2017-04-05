#pragma once

#include "common/aabr.hpp"
#include "common/time.hpp"
#include "graphics/blend_mode.hpp"
#include "graphics/cell/paint.hpp"
#include "graphics/cell/cell.hpp"
#include "graphics/scissor.hpp"

namespace notf {

class Cell;
class RenderContext;

namespace detail {

/** Empty base class in order to use the Painter enums both in PainterState and Painter.
 * Should be optimized away by the compiler.
 */
struct PainterBase {

    /** Type of cap used at the end of a painted line. */
    enum class LineCap : unsigned char {
        BUTT,
        ROUND,
        SQUARE,
    };

    /** Type of joint beween two painted line segments. */
    enum class LineJoin : unsigned char {
        MITER,
        ROUND,
        BEVEL,
    };

    /** Winding direction of a painted Shape */
    enum class Winding : unsigned char {
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE        = CW,
        SOLID            = CCW,
        HOLE             = CW,
    };
};

/** State used by a Painter (and Painterpreter) to contextualize paint operations. */
struct PainterState {
    Xform2f xform                   = Xform2f::identity();
    Scissor scissor                 = {Xform2f::identity(), {-1, -1}};
    BlendMode blend_mode            = BlendMode::SOURCE_OVER;
    PainterBase::LineCap line_cap   = PainterBase::LineCap::BUTT;
    PainterBase::LineJoin line_join = PainterBase::LineJoin::MITER;
    float alpha                     = 1;
    float miter_limit               = 10;
    float stroke_width              = 1;
    Paint fill_paint                = Color(255, 255, 255);
    Paint stroke_paint              = Color(0, 0, 0);
};

} // namespace detail

/**********************************************************************************************************************/

/**
 * The Painterpreter
 * =================
 * The Painter's job is to create Commands for the `Painterpreter`.
 * The Painterpreter trusts the Painter to only give correct values (no line width <0, no state underflow...).
 *
 * Paths
 * =====
 * Painting using the Painter class is done in several stages.
 * First, you define a "Path" using methods like `add_rect` and `add_circle`.
 * The combination of all Paths will be used to render the shape when calling `fill` or `stroke`.
 * In order to remove the current Path and start a new one call `begin_path`.
 * Calling `close_path` at the end of the Path definition is only necessary, if the current Shape is not already closed.
 * For example, if you construct a Path using bezier or quadratic curves.
 */
class Painter : public detail::PainterBase {

public: // methods

    friend class Widget;

    /** Value Constructor. */
    Painter(Cell& cell, RenderContext &context);

    /* State management ***********************************************************************************************/

    /** Copy the new state and place the copy on the stack.
     * @return  Current stack height.
     */
    size_t push_state();

    /** Restore the previous State from the stack.
     * Popping below the last State will have no effect.
     * @return  Current stack height.
     */
    size_t pop_state();

    /* Transform ******************************************/

    /** The Painter's current transform. */
    const Xform2f& get_transform() const { return _get_current_state().xform; }

    /** Sets the transform of the Painter. */
    void set_transform(const Xform2f xform);

    /** Reset the Painter's transform. */
    void reset_transform();

    /** Transforms the Painter's transformation matrix. */
    void transform(const Xform2f& transform);

    /** Translates the Painter's transformation matrix. */
    void translate(const float x, const float y) { translate(Vector2f{x, y}); }

    /** Translates the Painter's transformation matrix. */
    void translate(const Vector2f& delta);

    /** Rotates the current state the given amount of radians in a counter-clockwise direction. */
    void rotate(const float angle);

    /* Scissor ********************************************/

    /** The Scissor currently applied to the Painter. */
    Scissor get_scissor() const { return _get_current_state().scissor; }

    /** Updates Scissor currently applied to the Painter. */
    void set_scissor(const Aabrf& aabr);

    /** Removes the Scissor currently applied to the Painter. */
    void remove_scissor();

    /* Blend Mode *****************************************/

    /** The current Painter's blend mode. */
    BlendMode get_blend_mode() const { return _get_current_state().blend_mode; }

    /** Set the Painter's blend mode. */
    void set_blend_mode(const BlendMode mode);

    /* Alpha **********************************************/

    /** Get the global alpha for this Painter. */
    float get_alpha() const { return _get_current_state().alpha; }

    /** Set the global alpha for this Painter. */
    void set_alpha(const float alpha);

    /* Miter Limit ****************************************/

    /** The Painter's miter limit. */
    float get_miter_limit() const { return _get_current_state().miter_limit; }

    /** Sets the Painter's miter limit. */
    void set_miter_limit(const float limit);

    /* Line Cap *******************************************/

    /** The Painter's line cap. */
    LineCap get_line_cap() const { return _get_current_state().line_cap; }

    /** Sets the Painter's line cap. */
    void set_line_cap(const LineCap cap);

    /* Line Join ******************************************/

    /** The Painter's line join. */
    LineJoin get_line_join() const { return _get_current_state().line_join; }

    /** Sets the Painter's line join. */
    void set_line_join(const LineJoin join);

    /* Fill Paint *****************************************/

    /** The current fill Paint. */
    const Paint& get_fill_paint() { return _get_current_state().fill_paint; }

    /** Changes the current fill Paint. */
    void set_fill_paint(Paint paint);

    /** Changes the current fill Paint to a solid color. */
    void set_fill_color(Color color);

    /* Stroke Paint ***************************************/

    /** The current fill Paint. */
    const Paint& get_stroke_paint() { return _get_current_state().stroke_paint; }

    /** Changes the current stroke Paint. */
    void set_stroke_paint(Paint paint);

    /** Changes the current stroke Paint to a solid color. */
    void set_stroke_color(Color color);

    /** The stroke width of the Painter. */
    float get_stroke_width() const { return _get_current_state().stroke_width; }

    /** Changes the stroke width of the Painter. */
    void set_stroke_width(const float width);

    /* Paths *********************************************************************************************************/

    /** Clears the existing Path, but keeps the Painter's state intact. */
    void begin_path();

    /** Closes the current Path.
     * Has no effect on Paths that are already closed (like those you get from `add_rect` etc.).
     */
    void close_path();

    /** Changes the "Winding" of the current Path. */
    void set_winding(const Winding winding);

    /** Moves the stylus to a given position without creating a path. */
    void move_to(const float x, const float y) { move_to({x, y}); }
    void move_to(const Vector2f pos);

    /** Moves the stylus to a given position and creates a straight line. */
    void line_to(const float x, const float y) { line_to({x, y}); }
    void line_to(const Vector2f pos);

    /** Moves the stylus to `end` and draws a quadratic spline from the current postion over the given control point. */
    void quad_to(const float cx, const float cy, const float tx, const float ty);
    void quad_to(const Vector2f& ctrl, const Vector2f& end) { quad_to(ctrl.x, ctrl.y, end.x, end.y); }

    /** Moves the stylus to `end` and draws a bezier spline from the current postion over the two control points. */
    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty) { bezier_to({c1x, c1y}, {c2x, c2y}, {tx, ty}); }
    void bezier_to(const Vector2f ctrl1, const Vector2f ctrl2, const Vector2f end);

    /** Creates an arc Path, used to create parts of circles.
     * @see             https://www.w3schools.com/tags/canvas_arc.asp
     */
    void arc(const float x, const float y, const float r, const float start_angle, const float end_angle, const Winding dir = Winding::CCW);
    void arc(const Vector2f& center, const float radius, const float start_angle, const float end_angle, const Winding dir = Winding::CCW) { arc(center.x, center.y, radius, start_angle, end_angle, dir); }

    /** Create an open, arc between two tangents on the canvas.
     * @param tangent   Position defining the start tangent vector (from the current stylus position).
     * @param end       End position of the arc.
     * @param radius    Radius of the arc.
     * @see             http://www.w3schools.com/tags/canvas_arcto.asp
     */
    void arc_to(const Vector2f& tangent, const Vector2f& end, const float radius);
    void arc_to(const float x1, const float y1, const float x2, const float y2, const float radius) { arc_to({x1, y1}, {x2, y2}, radius); }

    /** Creates a new rectangular Path. */
    void add_rect(const float x, const float y, const float w, const float h);
    void add_rect(const Aabrf& rect) { add_rect(rect.left(), rect.top(), rect.width(), rect.height()); }

    /** Creates a new rectangular Path with rounded corners. */
    void add_rounded_rect(const float x, const float y, const float w, const float h, const float rtl, const float rtr, const float rbr, const float rbl);
    void add_rounded_rect(const Aabrf& rect, const float radius) { add_rounded_rect(rect.left(), rect.top(), rect.width(), rect.height(), radius, radius, radius, radius); }
    void add_rounded_rect(const float x, const float y, const float w, const float h, const float radius) { add_rounded_rect(x, y, w, h, radius, radius, radius, radius); }

    /** Creates a new elliptic Path. */
    void add_ellipse(const float cx, const float cy, const float rx, const float ry);
    void add_ellipse(const Vector2f& center, const Size2f& extend) { add_ellipse(center.x, center.y, extend.width, extend.height); }

    /** Creates a new circular Path. */
    void add_circle(const float cx, const float cy, const float radius) { add_ellipse(cx, cy, radius, radius); }
    void add_circle(const Vector2f& center, const float radius) { add_ellipse(center.x, center.y, radius, radius); }

    /* Painting *******************************************************************************************************/

    /** Fills the current Path with the Paint defined in the Painter's current State. */
    void fill();
    void fill_new();

    /** Strokes the current Path with the Paint defined in the Painter's current State. */
    void stroke();

    /* Context ********************************************************************************************************/

    /** Time at the beginning of the current frame. */
    Time get_time() const;

    /** The mouse position relative to the Window's top-left corner. */
    const Vector2f& get_mouse_pos() const;

//private: // methods
    /** The current State of the Painter. */
    detail::PainterState& _get_current_state()
    {
        assert(!s_states.empty());
        return s_states.back();
    }

    /** The current State of the Painter. */
    const detail::PainterState& _get_current_state() const
    {
        assert(!s_states.empty());
        return s_states.back();
    }

    void _flatten_paths();

    void _add_point(const Vector2f position, const Cell::Point::Flags flags);


    void _tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                           const float x3, const float y3, const float x4, const float y4);

//private: // fields
    /** Cell, that this Painter is painting into. */
    Cell& m_cell;

    /** The Render Context in which the Painter operates. */
    RenderContext& m_context;

    /** Current position of the 'stylus', as the last Command left it. */
    Vector2f m_stylus; // TODO: get rid of the stylus if possible

    /** Keeps track of whether the Painter has a current, open path or not.
     * If not, it has to create a new Path before addng Points.
     */
    bool m_has_open_path;

//private: // static fields
    /** Stack of all PainterStates of this Painter. */
    static std::vector<detail::PainterState> s_states;


    float poly_area(const std::vector<Cell::Point>& points, const size_t offset, const size_t count);

    void _expand_fill(const bool draw_antialiased);
    void _calculate_joins(const float fringe, const LineJoin join, const float miter_limit);

    void _bevel_join(const Cell::Point& previous_point, const Cell::Point& current_point, const float left_w, const float right_w,
                     const float left_u, const float right_u);


    std::tuple<float, float, float, float>
    choose_bevel(bool is_beveling, const Cell::Point& prev_point, const Cell::Point& curr_point, const float stroke_width);

};

} // namespace notf
