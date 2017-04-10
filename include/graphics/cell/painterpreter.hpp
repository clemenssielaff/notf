#pragma once

#include "graphics/cell/painter.hpp"

namespace notf {
using namespace detail;

class Cell;
class CellCanvas;
struct Vertex;

/**********************************************************************************************************************/

class Painterpreter {

private: // types
    struct Point {
        enum Flags : uint {
            NONE       = 0,
            CORNER     = 1u << 0,
            LEFT       = 1u << 1,
            BEVEL      = 1u << 2,
            INNERBEVEL = 1u << 3,
        };

        /** Position of the Point. */
        Vector2f pos;

        /** Direction to the next Point. */
        Vector2f forward;

        /** Miter distance. */
        Vector2f dm;

        /** Distance to the next point forward. */
        float length;

        /** Additional information about this Point. */
        Flags flags;
    };

    /******************************************************************************************************************/

    struct Path {
        /** Index of the first Point. */
        size_t first_point = 0;

        /** Number of Points in this Path. */
        size_t point_count = 0;

        /** What direction the Path is wound. */
        Painter::Winding winding = Painter::Winding::COUNTERCLOCKWISE;

        /** Whether this Path is closed or not.
         * Closed Paths will draw an additional line between their last and first Point.
         */
        bool is_closed = false;

        /** Whether the Path is convex or not. */
        bool is_convex = false;
    };

public: // methods
    /** Constructor. */
    Painterpreter(CellCanvas& context);

    /** Paints a given Cell. */
    void paint(Cell& cell);

private: // methods
    /** Resets the Painterpreter and clears all States, Points etc. */
    void _reset();

    /** The current State. */
    PainterState& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

    /** Copy the new state and place the copy on the stack. */
    void _push_state();

    /** Restore the previous State from the stack. */
    void _pop_state();

    /** Appends a new Point to the current Path. */
    void _add_point(Vector2f position, const Point::Flags flags);

    /** Creates a new, empty Path. */
    void _add_path();

    /** Calculates the area of a given Path. */
    float _get_path_area(const Path& path) const;

    /** Tesellates a Bezier curve.
     * Note that I am using the (experimental) improvement on the standard nanovg tesselation algorithm here,
     * as found at: https://github.com/memononen/nanovg/issues/328
     * If there seems to be an issue with the tesselation, revert back to the "official" implementation.
     */
    void _tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                           const float x3, const float y3, const float x4, const float y4);

    /** Chooses whether to bevel a joint or not and returns vertex coordinates. */
    std::tuple<float, float, float, float>
    _choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width);

    void _create_bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                            const float left_u, const float right_u, std::vector<Vertex>& vertices_out);

    void _create_round_join(const Point& previous_point, const Point& current_point, const float stroke_width,
                            const size_t divisions, std::vector<Vertex>& vertices_out);

    /** Creates the round cap and the start of a line.
     * @param point         Start point of the line.
     * @param delta         Direction of the cap.
     * @param stroke_width  Width of the base of the cap.
     * @param divisions     Number of divisions for the cap (per half-circle).
     * @param vertices_out  [out] Vertex vector to write the cap into.
     */
    void _create_round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width,
                                 const size_t divisions, std::vector<Vertex>& vertices_out);

    /** Creates the round cap and the end of a line.
     * @param point         End point of the line.
     * @param delta         Direction of the cap.
     * @param stroke_width  Width of the base of the cap.
     * @param divisions     Number of divisions for the cap (per half-circle).
     * @param vertices_out  [out] Vertex vector to write the cap into.
     */
    void _create_round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                               const size_t divisions, std::vector<Vertex>& vertices_out);

    void _create_butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width,
                                const float d, const float fringe_width, std::vector<Vertex>& vertices_out);

    void _create_butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                              const float d, const float fringe_width, std::vector<Vertex>& vertices_out);

    /** Remders a text at the given screen coordinate.
     * The position corresponts to the start of the text's baseline.
     */
    void _render_text(const std::string& text, const FontID font_id);

    /** Paints the current Path. */
    void _fill();

    /** Paints an outline of the current Path. */
    void _stroke();

    /** Analyzes the Points making up each Path to be drawn in `fill` or `stroke`. */
    void _prepare_paths(const float fringe, const Painter::LineJoin join, const float miter_limit);

public: // fields
    /** The Cell Context that is painted into. */
    CellCanvas& m_canvas;

    /** Points making up the Painter Paths. */
    std::vector<Point> m_points;

    /** Intermediate representation of the Painter Paths. */
    std::vector<Path> m_paths;

    /** Stack of painter states. */
    std::vector<PainterState> m_states;

    /** The bounds of all vertices, used to define the quad to render them onto. */
    Aabrf m_bounds;
};

} // namespace notf
