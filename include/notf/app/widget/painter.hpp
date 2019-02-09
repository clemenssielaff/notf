/* Painting Widgets is arguably the most important visual task of notf. In order to maximize performance, the Widget
 * drawing pipeline is divided into several stages, each of which helping to avoid unnecessary work.
 *
 * Painter
 * Is passed in as a mutable reference to `Widget::paint`, where it modifies the...
 *
 * WidgetDesign
 * A data object containing information on how to paint a Widget. Is used so that we don't have to call `Widget::paint`
 * for every Widget, every time a new frame is drawn but only for those that actually changed visually. The WidgetDesign
 * is basically bytecode that is eventually ingested by the...
 *
 * Painterpreter
 * A play on the words "Painter" and "Interpreter". The Painterpreter takes a look at a WidgetDesign and uses it to in
 * turn control the...
 *
 * Plotter
 * The Plotter behaves very much like the `Painter`, but instead of being driven by the user (with an interface suited
 * for human interaction) it is driven by the Painterpreter. The Plotter ingests all information necessary to draw a
 * WidgetScene, performs some internal optimizations and eventually passes them on to the GPU for rendering.
 *
 *
 *
 *
 *
 */
#pragma once

#include <vector>

#include "notf/meta/assert.hpp"
#include "notf/meta/pointer.hpp"

#include "notf/common/circle.hpp"
#include "notf/common/vector2.hpp"

#include "notf/graphic/renderer/plotter.hpp"

#include "notf/app/fwd.hpp"
#include "notf/app/widget/clipping.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

class Painter {

    friend Accessor<Painter, Painterpreter>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Painter);

    using Paint = Plotter::Paint;

    using PathId = Plotter::PathId;

    using PathPtr = Plotter::PathPtr;

private:
    /// State used to contextualize paint operations.
    struct State {
        Paint fill_paint = Color(255, 255, 255);
        Paint stroke_paint = Color(0, 0, 0);

        M3f xform = M3f::identity();

        /// Current Path.
        PathPtr path;

        /// Current Font.
        FontPtr font;

        Clipping clipping;

        float alpha = 1;
        float stroke_width = 1;

        // Limit of the ration of a joint's miter length to its stroke width.
        float miter_limit = 10;

        // Furthest distance between two points at which the second point is considered equal to the first.
        float distance_tolerance = precision_low<float>();

        BlendMode blend_mode = BlendMode::SOURCE_OVER;
        Paint::LineCap line_cap = Paint::LineCap::BUTT;
        Paint::LineJoin line_join = Paint::LineJoin::MITER;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param design   WidgetDesign to paint into (is overwritten).
    explicit Painter(WidgetDesign& design);

    // Paths ------------------------------------------------------------------

    /// Sets a new polygon as the current path.
    /// @param polygon  Polygon replacing the current Path.
    PathId set_path(Polygonf polygon);

    /// Sets a new spline as the current path.
    /// @param spline   Spline replacing the current Path.
    PathId set_path(CubicBezier2f spline);

    /// Makes a previously existing Path current again.
    /// If the given PathId does not represent an existing Path, this method has no effect.
    /// @param id   New current Path.
    /// @returns    Id of the current Path after this function has run.
    PathId set_path(PathId id);

    // Text -------------------------------------------------------------------

    /// Sets a new current Font.
    /// @param font     New current font.
    void set_font(FontPtr font);

    /// Renders a text.
    /// The transformation corresponds to the start of the text's baseline.
    void write(std::string text);

    // Painting ---------------------------------------------------------------

    /// Fills the current Path with the Paint defined in the Painter's current State.
    void fill();

    /// Strokes the current Path with the Paint defined in the Painter's current State.
    void stroke();

    // Transform --------------------------------------------------------------

    /// The Painter's current transform.
    const M3f& get_transform() const { return _get_current_state().xform; }

    /// Sets the transform of the Painter.
    void set_transform(const M3f& transform);

    /// Reset the Painter's transform.
    void reset_transform();

    /// Transforms the Painter's transformation matrix.
    void transform(const M3f& transform);

    /// Translates the Painter's transformation matrix.
    void translate(const float x, const float y) { translate(V2f{x, y}); }

    /// Translates the Painter's transformation matrix.
    void translate(const V2f& delta);

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
    void set_fill(Color color) { set_fill(Paint(std::move(color))); }

    // Stroke Paint -----------------------------------------------------------

    /// The current fill Paint.
    const Paint& get_stroke() { return _get_current_state().stroke_paint; }

    /// The stroke width of the Painter.
    float get_stroke_width() const { return _get_current_state().stroke_width; }

    /// Changes the current stroke Paint.
    void set_stroke(Paint paint);

    /// Changes the current stroke Paint to a solid color.
    void set_stroke(Color color) { set_stroke(Paint(std::move(color))); }

    /// Changes the stroke width of the Painter.
    void set_stroke_width(float width);

private:
    /// The current State of the Painter.
    State& _get_current_state() {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }

    /// The current State of the Painter.
    const State& _get_current_state() const {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Stack of states.
    std::vector<State> m_states = {State()};

    /// WidgetDesign to paint into.
    WidgetDesign& m_design;

    /// Id of the current Path.
    PathId m_current_path_id = PathId::invalid();

    /// Id of the next generated Path.
    PathId::underlying_t m_next_path_id = PathId::first().get_value();
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<Painter, Painterpreter> {
    friend Painterpreter;

    /// State used to contextualize paint operations.
    using State = notf::Painter::State;
};

NOTF_CLOSE_NAMESPACE
