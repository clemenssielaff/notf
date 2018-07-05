#pragma once

#include "app/widget/painter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Painterpreter {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// State used to contextualize paint operations.
    using State = Painter::Access<Painterpreter>::State;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param context      The GraphicsContext containing the graphic objects.
    Painterpreter(GraphicsContext& context);

    /// Paints the Design of the given Widget.
    void paint(const Widget& widget);

private:
    /// The current State.
    State& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

    /// Copy the new state and place the copy on the stack.
    void _push_state();

    /// Restore the previous State from the stack.
    void _pop_state();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Stack of states.
    std::vector<State> m_states = {State()};

    /// First transforamtion applied all paint operations.
    Matrix3f m_base_xform = Matrix3f::identity();

    /// Plotter used to render the Designs to the screen.
    PlotterPtr m_plotter;

    /// Clipping applied to all paint operations.
    Clipping m_base_clipping = {};
};

NOTF_CLOSE_NAMESPACE
