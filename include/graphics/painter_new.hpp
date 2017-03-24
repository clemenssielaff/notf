#pragma once

#include "common/aabr.hpp"
#include "common/color.hpp"
#include "common/size2.hpp"
#include "common/xform2.hpp"
#include "graphics/blend_mode.hpp"

namespace notf {

class Cell;
class RenderContext;
class Texture2;

/**********************************************************************************************************************/

/** A Paint is an object used by a Painter to draw into a Cell.
 * Most of the paint fields are used to initialize the Fragment uniform in the Cell shader.
 */
struct Paint {
    /** Default Constructor. */
    Paint() = default;

    /** Value Constructor with a single Color. */
    Paint(Color color)
        : xform(Xform2f::identity())
        , extent()
        , radius(0)
        , feather(1)
        , inner_color(std::move(color))
        , outer_color(inner_color)
        , texture()
    {
    }

public: // static methods
    static Paint create_linear_gradient(const Vector2f& start_pos, const Vector2f& end_pos,
                                        const Color start_color, const Color end_color);

    static Paint create_radial_gradient(const Vector2f& center,
                                        const float inner_radius, const float outer_radius,
                                        const Color inner_color, const Color outer_color);

    static Paint create_box_gradient(const Vector2f& center, const Size2f& extend,
                                     const float radius, const float feather,
                                     const Color inner_color, const Color outer_color);

    static Paint create_texture_pattern(const Vector2f& top_left, const Size2f& extend,
                                        std::shared_ptr<Texture2> texture,
                                        const float angle, const float alpha);

    /** Turns the Paint into a single solid. */
    void set_color(const Color color)
    {
        xform       = Xform2f::identity();
        radius      = 0;
        feather     = 1;
        inner_color = std::move(color);
        outer_color = inner_color;
    }

    /** Local transform of the Paint. */
    Xform2f xform;

    /** Extend of the Paint. */
    Size2f extent;

    float radius;

    float feather;

    Color inner_color;

    Color outer_color;

    std::shared_ptr<Texture2> texture;
};

/**********************************************************************************************************************/

/**
 * Paths
 * =====
 * Painting using the Painter class is done in several stages.
 * First, you define a "Path" using methods like `add_rect` and `add_circle`.
 * The combination of all Paths will be used to render the shape when calling `fill` or `stroke`.
 * In order to remove the current Path and start a new one call `begin_path`.
 * Calling `close_path` at the end of the Path definition is only necessary, if the current Shape is not already closed.
 * For example, if you construct a Path using bezier or quadratic curves.
 */
class Painter {

public: // enums
    /******************************************************************************************************************/

    struct Scissor {
        /** Scissors have their own transformation. */
        Xform2f xform;

        /** Extend around the center of the Transform. */
        Size2f extend;
    };

    /******************************************************************************************************************/

    enum class LineCap : unsigned char {
        BUTT,
        ROUND,
        SQUARE,
    };

    /******************************************************************************************************************/

    enum class LineJoin : unsigned char {
        MITER,
        ROUND,
        BEVEL,
    };

    /******************************************************************************************************************/

    enum class Winding : unsigned char {
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE        = CW,
        SOLID            = CCW,
        HOLE             = CW,
    };

private: // types
    /******************************************************************************************************************/

    struct State {
        State()
            : xform(Xform2f::identity())
            , scissor({Xform2f::identity(), {-1, -1}})
            , blend_mode(BlendMode::SOURCE_OVER)
            , line_cap(LineCap::BUTT)
            , line_join(LineJoin::MITER)
            , alpha(1)
            , miter_limit(10)
            , stroke_width(1)
            , previous_state(INVALID_INDEX)
            , fill(Color(255, 255, 255))
            , stroke(Color(0, 0, 0))
        {
        }

        State(const State& other) = default;

        Xform2f xform;
        Scissor scissor;
        BlendMode blend_mode;
        LineCap line_cap;
        LineJoin line_join;
        float alpha;
        float miter_limit;
        float stroke_width;
        size_t previous_state;
        Paint fill;
        Paint stroke;

        static const size_t INVALID_INDEX = std::numeric_limits<size_t>::max();
    };

private: // constructor
    /******************************************************************************************************************/

    /** Value Constructor. */
    Painter(Cell& cell, const RenderContext& context);

public: // methods
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
    void set_transform(const Xform2f xform) { _get_current_state().xform = std::move(xform); }

    /** Reset the Painter's transform. */
    void reset_transform() { set_transform(Xform2f::identity()); }

    /** Transforms the Painter's transformation matrix. */
    void transform(const Xform2f& transform) { _get_current_state().xform *= transform; }

    /** Translates the Painter's transformation matrix. */
    void translate(const float x, const float y) { translate(Vector2f{x, y}); }

    /** Translates the Painter's transformation matrix. */
    void translate(const Vector2f delta) { _get_current_state().xform *= Xform2f::translation(std::move(delta)); }

    /** Rotates the current state the given amount of radians in a counter-clockwise direction. */
    void rotate(const float angle) { _get_current_state().xform = Xform2f::rotation(angle) * _get_current_state().xform; }
    // TODO: Transform2::premultiply

    /* Scissor ********************************************/

    /** The Scissor currently applied to the Painter. */
    Scissor get_scissor() const { return _get_current_state().scissor; }

    /** Updates Scissor currently applied to the Painter. */
    void set_scissor(const Aabrf& aabr);

    /** Removes the Scissor currently applied to the Painter. */
    void remove_scissor() { _get_current_state().scissor = {Xform2f::identity(), {-1, -1}}; }

    /* Blend Mode *****************************************/

    /** The current Painter's blend mode. */
    BlendMode get_blend_mode() const { return _get_current_state().blend_mode; }

    /** Set the Painter's blend mode. */
    void set_blend_mode(const BlendMode mode) { _get_current_state().blend_mode = std::move(mode); }

    /* Alpha **********************************************/

    /** Get the global alpha for this Painter. */
    float get_alpha() const { return _get_current_state().alpha; }

    /** Set the global alpha for this Painter. */
    void set_alpha(const float alpha) { _get_current_state().alpha = alpha; }

    /* Miter Limit ****************************************/

    /** The Painter's miter limit. */
    float get_miter_limit() const { return _get_current_state().miter_limit; }

    /** Sets the Painter's miter limit. */
    void set_miter_limit(const float limit) { _get_current_state().miter_limit = limit; }

    /* Line Cap *******************************************/

    /** The Painter's line cap. */
    LineCap get_line_cap() const { return _get_current_state().line_cap; }

    /** Sets the Painter's line cap. */
    void set_line_cap(const LineCap cap) { _get_current_state().line_cap = cap; }

    /* Line Join ******************************************/

    /** The Painter's line join. */
    LineJoin get_line_join() const { return _get_current_state().line_join; }

    /** Sets the Painter's line join. */
    void set_line_join(const LineJoin join) { _get_current_state().line_join = join; }

    /* Fill Paint *****************************************/

    /** Mutable reference to the current fill Paint. */
    Paint& get_fill_paint() { return _get_current_state().fill; }

    /* Stroke Paint ***************************************/

    /** Mutable reference to the current fill Paint. */
    Paint& get_stroke_paint() { return _get_current_state().stroke; }

    /** The stroke width of the Painter. */
    float get_stroke_width() const { return _get_current_state().stroke_width; }

    /** Changes the stroke width of the Painter. */
    void set_stroke_width(const float width) { _get_current_state().stroke_width = max(0.f, width); }

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
    void move_to(const float x, const float y);
    void move_to(const Vector2f& pos) { move_to(pos.x, pos.y); }

    /** Moves the stylus to a given position and creates a straight line. */
    void line_to(const float x, const float y);
    void line_to(const Vector2f& pos) { line_to(pos.x, pos.y); }

    /** Moves the stylus to `end` and draws a quadratic spline from the current postion over the given control point. */
    void quad_to(const float cx, const float cy, const float tx, const float ty);
    void quad_to(const Vector2f& ctrl, const Vector2f& end) { quad_to(ctrl.x, ctrl.y, end.x, end.y); }

    /** Moves the stylus to `end` and draws a bezier spline from the current postion over the two control points. */
    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty);
    void bezier_to(const Vector2f& ctrl1, const Vector2f& ctrl2, const Vector2f& end) { bezier_to(ctrl1.x, ctrl1.y, ctrl2.x, ctrl2.y, end.x, end.y); }

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

    /** Strokes the current Path with the Paint defined in the Painter's current State. */
    void stroke();

private: // methods
    /** The current State of the Painter. */
    State& _get_current_state()
    {
        assert(m_state_index < s_states.size());
        return s_states[m_state_index];
    }

    /** The current State of the Painter. */
    const State& _get_current_state() const
    {
        assert(m_state_index < s_states.size());
        return s_states[m_state_index];
    }

    /** Appends new Commands to the buffer. */
    void _append_commands(std::vector<float>&& commands);

private: // methods for friends
    /** Clear the Painter's Cell, executes the Command stack and performs the drawings. */
    void _execute();

private: // fields
    /** Cell, that this Painter is painting into. */
    Cell& m_cell;

    /** Current position of the 'stylus', as the last Command left it. */
    Vector2f m_stylus;

    /** Index of the current State in `s_states`. */
    size_t m_state_index;

private: // static fields
    /** All Painter States used by this Painter, their order is determined by `m_state_succession`. */
    static std::vector<State> s_states;

    /** Order in which the States are traversed when the Painter is executed. */
    static std::vector<size_t> s_state_succession;
};

} // namespace notf
