#pragma once

#include "notf/graphic/plotter/plotter.hpp"

NOTF_OPEN_NAMESPACE

// painter ========================================================================================================== //

class Painter {

    // types ----------------------------------------------------------------------------------- //
public:
    using Paint = Plotter::Paint;
    using CapStyle = Plotter::CapStyle;
    using JointStyle = Plotter::JointStyle;

private:
    using State = Plotter::PainterState;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param design   WidgetDesign to paint into (current Design is overwritten).
    explicit Painter(PlotterDesign& design);

    /// Destructor.
    ~Painter();

    /// Pushes a copy of the current state of the Painter onto the state stack.
    /// All changes made to the state from now on can be undone by popping the state stack again.
    void push_state();

    /// Removes the topmost state from the state stack and applies the one below.
    /// If the current state is the only one left, this method is equivalent to calling `reset_state`.
    void pop_state();

    /// Resets the current state of the Painter without affecting the state stack.
    void reset_state();

    // setup ------------------------------------------------------------------

    /// Current Path to fill / stroke.
    const Path2Ptr& get_path() const { return _get_state().path; }

    /// Sets a new Path to fill / stroke.
    /// @param path New path.
    void set_path(Path2Ptr path);

    /// Current Font used for writing.
    const FontPtr& get_font() const { return _get_state().font; }

    /// Sets a new current Font.
    /// @param font     New current font.
    void set_font(FontPtr font);

    /// The current Paint.
    const Paint& get_paint() { return _get_state().paint; }

    /// Changes the current Paint.
    void set_paint(Paint paint);

    /// The clip currently applied to the Painter.
    const Aabrf& get_clip() const { return _get_state().clip; }

    /// Updates the Painter's clip.
    void set_clip(Aabrf clip);

    /// Removes the Painter's clip.
    void remove_clip() { set_clip({}); }

    // transformation ---------------------------------------------------------

    /// The Painter's current transform.
    const M3f& get_transform() const { return _get_state().xform; }

    /// Sets the transform of the Painter.
    void set_transform(const M3f& xform);

    /// Reset the Painter's transform.
    void reset_transform() { set_transform(M3f::identity()); }

    /// Transforms the Painter with the given 2D transformation matrix.
    /// @param xform    2D transformation matrix.
    /// @returns        This Painter after the transformation.
    Painter& operator*=(const M3f& xform);

    // rendering --------------------------------------------------------------

    /// Fills the current Path with the Paint defined in the Painter's current State.
    void fill();

    /// Strokes the current Path with the Paint defined in the Painter's current State.
    void stroke();

    /// Renders a text.
    /// The transformation corresponds to the start of the text's baseline.
    void write(std::string text);

    // detail -----------------------------------------------------------------

    /// The Painter's current blend mode.
    BlendMode get_blend_mode() const { return _get_state().blend_mode; }

    /// Set the Painter's blend mode.
    void set_blend_mode(const BlendMode mode);

    /// Get the global alpha for this Painter.
    float get_alpha() const { return _get_state().alpha; }

    /// Set the global alpha for this Painter.
    void set_alpha(const float alpha);

    /// The Painter's line cap.
    CapStyle get_line_cap() const { return _get_state().line_cap; }

    /// Sets the Painter's line cap.
    void set_cap_style(const CapStyle cap); // TODO: set_x for both cap and joints, overloade on argument type?

    /// The Painter's line join.
    JointStyle get_line_join() const { return _get_state().joint_style; }

    /// Sets the Painter's line join.
    void set_joint_style(const JointStyle join);

    /// The stroke width of the Painter.
    float get_stroke_width() const { return _get_state().stroke_width; }

    /// Changes the stroke width of the Painter.
    void set_stroke_width(const float stroke_width);

private:
    /// @{
    /// The current State of the Painter.
    const State& _get_state() const {
        NOTF_ASSERT(!m_states.empty());
        return m_states.back();
    }
    State& _get_state() { return NOTF_FORWARD_CONST_AS_MUTABLE(_get_state()); }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Stack of states.
    std::vector<State> m_states = {State()};

    /// PlotterDesign to paint into.
    PlotterDesign& m_design;
};

NOTF_CLOSE_NAMESPACE
