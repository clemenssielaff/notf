#pragma once

#include <vector>

#include "app/forwards.hpp"
#include "app/widget/clipping.hpp"
#include "app/widget/paint.hpp"
#include "common/assert.hpp"
#include "common/circle.hpp"
#include "common/vector2.hpp"
#include "graphics/core/gl_modes.hpp"

NOTF_OPEN_NAMESPACE

namespace access {
template<class>
class _Painter;
} // namespace access

// ================================================================================================================== //

class Painter {

    friend class access::_Painter<Painterpreter>;

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// State used to contextualize paint operations.
    struct State {
        Matrix3f xform = Matrix3f::identity();
        Clipping clipping = {};
        Paint fill_paint = Color(255, 255, 255);
        Paint stroke_paint = Color(0, 0, 0);
        BlendMode blend_mode = BlendMode::SOURCE_OVER;
        Paint::LineCap line_cap = Paint::LineCap::BUTT;
        Paint::LineJoin line_join = Paint::LineJoin::MITER;
        float alpha = 1;
        float stroke_width = 1;

        // Limit of the ration of a joint's miter length to its stroke width.
        float miter_limit = 10;

        // Furthest distance between two points in which the second point is considered equal to the first.
        float distance_tolerance;
    };

public:
    /// Access types.
    template<class T>
    using Access = access::_Painter<T>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param design   WidgetDesign to paint into (is overwritten).
    explicit Painter(WidgetDesign& design);

    // State Management -------------------------------------------------------

    /// Copy the current state and place the copy on the stack.
    /// @return  Current stack height.
    size_t push_state();

    /// Restore the previous state from the stack.
    /// Popping below the last state will have no effect.
    /// @return  Current stack height.
    size_t pop_state();

    // Transform --------------------------------------------------------------

    /// The Painter's current transform.
    const Matrix3f& get_transform() const { return _get_current_state().xform; }

    /// Sets the transform of the Painter.
    void set_transform(const Matrix3f& xform);

    /// Reset the Painter's transform.
    void reset_transform();

    /// Transforms the Painter's transformation matrix.
    void transform(const Matrix3f& transform);

    /// Translates the Painter's transformation matrix.
    void translate(const float x, const float y) { translate(Vector2f{x, y}); }

    /// Translates the Painter's transformation matrix.
    void translate(const Vector2f& delta);

    /// Rotates the current state the given amount of radians in a counter-clockwise direction.
    void rotate(const float angle);

    // Clipping ---------------------------------------------------------------

    /// The Clipping currently applied to the Painter.
    const Clipping& get_clipping() const { return _get_current_state().clipping; }

    /// Updates the Painter's Clipping.
    void set_clipping(Clipping clipping);

    /// Removes the Painter's Clipping rect.
    void remove_clipping();

    // Blend Mode -------------------------------------------------------------

    /// The Painter's current blend mode.
    BlendMode get_blend_mode() const { return _get_current_state().blend_mode; }

    /// Set the Painter's blend mode.
    void set_blend_mode(const BlendMode mode);

    // Alpha ------------------------------------------------------------------

    /// Get the global alpha for this Painter.
    float get_alpha() const { return _get_current_state().alpha; }

    /// Set the global alpha for this Painter.
    void set_alpha(const float alpha);

    // Miter Limit ------------------------------------------------------------

    /// The Painter's miter limit.
    float get_miter_limit() const { return _get_current_state().miter_limit; }

    /// Sets the Painter's miter limit.
    void set_miter_limit(const float limit);

    // Line Cap ---------------------------------------------------------------

    /// The Painter's line cap.
    Paint::LineCap get_line_cap() const { return _get_current_state().line_cap; }

    /// Sets the Painter's line cap.
    void set_line_cap(const Paint::LineCap cap);

    // Line Join --------------------------------------------------------------

    /// The Painter's line join.
    Paint::LineJoin get_line_join() const { return _get_current_state().line_join; }

    /// Sets the Painter's line join.
    void set_line_join(const Paint::LineJoin join);

    // Fill Paint -------------------------------------------------------------

    /// The current fill Paint.
    const Paint& get_fill() { return _get_current_state().fill_paint; }

    /// Changes the current fill Paint.
    void set_fill(Paint paint);

    /// Changes the current fill Paint to a solid color.
    void set_fill(Color color);

    // Stroke Paint -----------------------------------------------------------

    /// The current fill Paint.
    const Paint& get_stroke() { return _get_current_state().stroke_paint; }

    /// The stroke width of the Painter.
    float get_stroke_width() const { return _get_current_state().stroke_width; }

    /// Changes the current stroke Paint.
    void set_stroke(Paint paint);

    /// Changes the current stroke Paint to a solid color.
    void set_stroke(Color color);

    /// Changes the stroke width of the Painter.
    void set_stroke_width(const float width);

    // Paths ------------------------------------------------------------------

    /// Clears the existing Path, but keeps the Painter's state intact.
    void begin_path();

    /// Closes the current Path.
    /// Has no effect on Paths that are already closed(like those you get from `add_rect` etc.).
    void close_path();

    /// Changes the "Winding" of the current Path.
    void set_winding(const Paint::Winding winding);

    /// Moves the stylus to a given position without creating a path.
    void move_to(const float x, const float y) { move_to({x, y}); }
    void move_to(Vector2f pos);

    /// Moves the stylus to a given position and creates a straight line.
    void line_to(const float x, const float y) { line_to({x, y}); }
    void line_to(Vector2f pos);

    /// Moves the stylus to `end` and draws a quadratic spline from the current postion over the given control point.
    void quad_to(const float cx, const float cy, const float tx, const float ty);
    void quad_to(const Vector2f& ctrl, const Vector2f& end) { quad_to(ctrl.x(), ctrl.y(), end.x(), end.y()); }

    /// Moves the stylus to `end` and draws a bezier spline from the current postion over the two control points.
    void bezier_to(const float c1x, const float c1y, const float c2x, const float c2y, const float tx, const float ty)
    {
        bezier_to({c1x, c1y}, {c2x, c2y}, {tx, ty});
    }
    void bezier_to(Vector2f ctrl1, Vector2f ctrl2, Vector2f end);

    /// Creates an arc Path, used to create parts of circles.
    /// @see https://www.w3schools.com/tags/canvas_arc.asp
    void arc(const float x, const float y, const float r, const float start_angle, const float end_angle,
             const Paint::Winding dir = Paint::Winding::CCW);
    void arc(const Vector2f& center, const float radius, const float start_angle, const float end_angle,
             const Paint::Winding dir = Paint::Winding::CCW)
    {
        arc(center.x(), center.y(), radius, start_angle, end_angle, dir);
    }
    void arc(const Circlef& circle, const float start_angle, const float end_angle,
             const Paint::Winding dir = Paint::Winding::CCW)
    {
        arc(circle.center.x(), circle.center.y(), circle.radius, start_angle, end_angle, dir);
    }

    /// Create an open, arc between two tangents on the canvas.
    /// @param tangent  Position defining the start tangent vector(from the current stylus position).
    /// @param end      End position of the arc.
    /// @param radius   Radius of the arc.
    /// @see http://www.w3schools.com/tags/canvas_arcto.asp
    void arc_to(const Vector2f& tangent, const Vector2f& end, const float radius);
    void arc_to(const float x1, const float y1, const float x2, const float y2, const float radius)
    {
        arc_to({x1, y1}, {x2, y2}, radius);
    }

    /// Creates a new rectangular Path.
    void add_rect(const float x, const float y, const float w, const float h);
    void add_rect(const Aabrf& rect)
    {
        add_rect(rect.get_left(), rect.get_bottom(), rect.get_width(), rect.get_height());
    }

    /// Creates a new rectangular Path with rounded corners.
    void add_rounded_rect(const float x, const float y, const float w, const float h, const float rtl, const float rtr,
                          const float rbr, const float rbl);
    void add_rounded_rect(const Aabrf& rect, const float radius)
    {
        add_rounded_rect(rect.get_left(), rect.get_bottom(), rect.get_width(), rect.get_height(), radius, radius,
                         radius, radius);
    }
    void add_rounded_rect(const float x, const float y, const float w, const float h, const float radius)
    {
        add_rounded_rect(x, y, w, h, radius, radius, radius, radius);
    }
    void add_rounded_rect(const Aabrf& rect, const float rtl, const float rtr, const float rbr, const float rbl)
    {
        add_rounded_rect(rect.get_left(), rect.get_bottom(), rect.get_width(), rect.get_height(), rtl, rtr, rbr, rbl);
    }

    /// Creates a new elliptic Path.
    void add_ellipse(const float cx, const float cy, const float rx, const float ry);
    void add_ellipse(const Vector2f& center, const Size2f& extend)
    {
        add_ellipse(center.x(), center.y(), extend.width, extend.height);
    }

    /// Creates a new circular Path.
    void add_circle(const float cx, const float cy, const float radius) { add_ellipse(cx, cy, radius, radius); }
    void add_circle(const Vector2f& center, const float radius) { add_ellipse(center.x(), center.y(), radius, radius); }
    void add_circle(const Circlef& circle)
    {
        add_ellipse(circle.center.x(), circle.center.y(), circle.radius, circle.radius);
    }

    // Text -------------------------------------------------------------------

    /// Renders a text.
    /// The stencil position corresponts to the start of the text's baseline.
    void write(const std::string& text, const std::shared_ptr<Font> font);

    // Painting ---------------------------------------------------------------

    /// Fills the current Path with the Paint defined in the Painter's current State.
    void fill();

    /// Strokes the current Path with the Paint defined in the Painter's current State.
    void stroke();

private:
    /// The current State of the Painter.
    State& _get_current_state()
    {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }

    /// The current State of the Painter.
    const State& _get_current_state() const
    {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Stack of states.
    std::vector<State> m_states = {State()};

    /// WidgetDesign to paint into.
    WidgetDesign& m_design;

    /// Current position of the 'stylus', as the last Command left it.
    Vector2f m_stylus = Vector2f::zero();

    /// Keeps track of whether the Painter has a current, open path or not.
    /// If not, it has to create a new Path before addng Points.
    bool m_has_open_path = false;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class access::_Painter<Painterpreter> {
    friend class notf::Painterpreter;

    /// State used to contextualize paint operations.
    using State = notf::Painter::State;
};

NOTF_CLOSE_NAMESPACE
