#pragma once

#include "app/widget/painter.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class Painterpreter {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    /// State used to contextualize paint operations.
    using State = Painter::Access<Painterpreter>::State;

    // ========================================================================

    struct Point {
        enum Flags : uint {
            NONE = 0,
            CORNER = 1u << 0,
            LEFT = 1u << 1,
            BEVEL = 1u << 2,
            INNERBEVEL = 1u << 3,
        };

        /// Position of the Point.
        Vector2f pos;

        /// Direction to the next Point.
        Vector2f forward;

        /// Miter distance.
        Vector2f dm;

        /// Distance to the next point forward.
        float length;

        /// Additional information about this Point.
        Flags flags;
    };

    // ========================================================================

    struct Path {
        /// Index of the first Point.
        size_t first_point = 0;

        /// Number of Points in this Path.
        size_t point_count = 0;

        /// What direction the Path is wound.
        Paint::Winding winding = Paint::Winding::SOLID;

        /// Whether this Path is closed or not.
        /// Closed Paths will draw an additional line between their last and first Point.
        bool is_closed = false;

        /// Whether the Path is convex or not.
        bool is_convex = false;
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param context      The GraphicsContext containing the graphic objects.
    Painterpreter(GraphicsContext& context);

    /// Paints the Design of the given Widget.
    void paint(Widget& widget);

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

    /// Appends a new Point to the current Path.
    void _add_point(Vector2f position, const Point::Flags flags);

    /// Creates a new, empty Path.
    void _create_path();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Plotter used to render the Designs to the screen.
    PlotterPtr m_plotter;

    /// Stack of states.
    std::vector<State> m_states;

    //// All points in the Painterpreter.
    std::vector<Point> m_points;

    /// All Paths stored in the Painterpreter (they refer to Points in `m_points`).
    std::vector<Path> m_paths;

    /// The Widget's window transform.
    Matrix3f m_base_xform;

    /// Clipping provided by the Widget.
    Clipping m_base_clipping;

    /// The bounds of all vertices of a path, used to define the quad to render them onto.
    Aabrf m_bounds; // TODO: do I really need this? maybe it's better to draw all polygons multiple times than to
                    // overdraw
};

NOTF_CLOSE_NAMESPACE
