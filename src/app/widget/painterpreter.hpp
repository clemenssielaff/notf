#pragma once

#include "app/widget/painter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Painterpreter {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// State used to contextualize paint operations.
    using State = Painter::Access<Painterpreter>::State;

    using Paint = Plotter::Paint;

    using PathPtr = Plotter::PathPtr;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param context      The GraphicsContext containing the graphic objects.
    Painterpreter(GraphicsContext& context);

    /// Paints the Design of the given Widget.
    void paint(Widget& widget);

private:
    /// Reset the internal state.
    void _reset();

    /// The current State.
    State& _get_current_state();

    /// Copy the new state and place the copy on the stack.
    void _push_state();

    /// Restore the previous State from the stack.
    void _pop_state();

    /// Creates a new, empty Path.
    void _create_path();

    /// Paints the current Path.
    void _fill();

    /// Paints an outline of the current Path.
    void _stroke();

    /// Writes the given text.
    void _write(const std::string& text);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Plotter used to render the Designs to the screen.
    PlotterPtr m_plotter;

    /// Stack of states.
    std::vector<State> m_states;

    /// The Widget's window transform.
    Matrix3f m_base_xform;

    /// Clipping provided by the Widget.
    Clipping m_base_clipping;

    /// The bounds of all vertices of a path, used to define the quad to render them onto.
    Aabrf m_bounds; // TODO: do I really need this? maybe it's better to draw all polygons multiple times than to
                    // overdraw

    /// All Paths created by the Widget, addressable by index.
    std::vector<valid_ptr<PathPtr>> m_paths;
};

NOTF_CLOSE_NAMESPACE
