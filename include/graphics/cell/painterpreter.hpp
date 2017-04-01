#pragma once

#include <array>

#include "common/vector2.hpp"
#include "graphics/cell/painter.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {
using namespace detail;

class Cell;
struct Vertex;
class RenderContext;

/**********************************************************************************************************************/

struct Command {
    using command_t = float; // TODO: use _Vector2<command_t> instead of Vector2f?

    enum Type : uint32_t {
        PUSH_STATE,
        POP_STATE,
        BEGIN_PATH,
        SET_WINDING,
        CLOSE,
        MOVE,
        LINE,
        BEZIER,
        FILL,
        STROKE,
        SET_XFORM,
        RESET_XFORM,
        TRANSFORM,
        TRANSLATE,
        ROTATE,
        SET_SCISSOR,
        RESET_SCISSOR,
        SET_FILL_COLOR,
        SET_FILL_PAINT,
        SET_STROKE_COLOR,
        SET_STROKE_PAINT,
        SET_STROKE_WIDTH,
        SET_BLEND_MODE,
        SET_ALPHA,
        SET_MITER_LIMIT,
        SET_LINE_CAP,
        SET_LINE_JOIN,
    };

protected: // method
    Command(Type type)
        : type(static_cast<command_t>(to_number(type))) {}

public: // field
    const command_t type;
};

/** Command to copy the current PainterState and push it on the states stack. */
struct PushStateCommand : public Command {
    PushStateCommand()
        : Command(PUSH_STATE) {}
};

/** Command to remove the current PainterState and back to the previous one. */
struct PopStateCommand : public Command {
    PopStateCommand()
        : Command(POP_STATE) {}
};

/** Command to start a new path. */
struct BeginCommand : public Command {
    BeginCommand()
        : Command(BEGIN_PATH) {}
};

/** Command setting the winding direction for the next fill or stroke. */
struct SetWindingCommand : public Command {
    SetWindingCommand(Painter::Winding winding)
        : Command(SET_WINDING), winding(static_cast<float>(to_number(winding))) {}
    float winding;
};

/** Command to close the current path. */
struct CloseCommand : public Command {
    CloseCommand()
        : Command(CLOSE) {}
};

/** Command to move the Painter's stylus without drawing a line.
 * Creates a new path.
 */
struct MoveCommand : public Command {
    MoveCommand(Vector2f pos)
        : Command(MOVE), pos(std::move(pos)) {}
    Vector2f pos;
};

/** Command to draw a line from the current stylus position to the one given. */
struct LineCommand : public Command {
    LineCommand(Vector2f pos)
        : Command(LINE), pos(std::move(pos)) {}
    Vector2f pos;
};

/** Command to draw a bezier spline from the current stylus position. */
struct BezierCommand : public Command {
    BezierCommand(Vector2f ctrl1, Vector2f ctrl2, Vector2f end)
        : Command(BEZIER), ctrl1(std::move(ctrl1)), ctrl2(std::move(ctrl2)), end(std::move(end)) {}
    Vector2f ctrl1;
    Vector2f ctrl2;
    Vector2f end;
};

/** Command to fill the current paths using the current PainterState. */
struct FillCommand : public Command {
    FillCommand()
        : Command(FILL) {}
};

/** Command to stroke the current paths using the current PainterState. */
struct StrokeCommand : public Command {
    StrokeCommand()
        : Command(STROKE) {}
};

/** Command to change the Xform of the current PainterState. */
struct SetXformCommand : public Command {
    SetXformCommand(Xform2f xform)
        : Command(SET_XFORM), xform(std::move(xform)) {}
    Xform2f xform;
};

/** Command to reset the Xform of the current PainterState. */
struct ResetXformCommand : public Command {
    ResetXformCommand()
        : Command(RESET_XFORM) {}
};

/** Command to transform the current Xform of the current PainterState. */
struct TransformCommand : public Command {
    TransformCommand(Xform2f xform)
        : Command(TRANSFORM), xform(std::move(xform)) {}
    Xform2f xform;
};

/** Command to add a translation the Xform of the current PainterState. */
struct TranslationCommand : public Command {
    TranslationCommand(Vector2f delta)
        : Command(TRANSLATE), delta(std::move(delta)) {}
    Vector2f delta;
};

/** Command to add a rotation in radians to the Xform of the current PainterState. */
struct RotationCommand : public Command {
    RotationCommand(float angle)
        : Command(ROTATE), angle(angle) {}
    float angle;
};

/** Command to set the Scissor of the current PainterState. */
struct SetScissorCommand : public Command {
    SetScissorCommand(Scissor sissor)
        : Command(SET_SCISSOR), sissor(sissor) {}
    Scissor sissor;
};

/** Command to reset the Scissor of the current PainterState. */
struct ResetScissorCommand : public Command {
    ResetScissorCommand()
        : Command(RESET_SCISSOR) {}
};

/** Command to set the fill Color of the current PainterState. */
struct FillColorCommand : public Command {
    FillColorCommand(Color color)
        : Command(SET_FILL_COLOR), color(color) {}
    Color color;
};

/** Command to set the fill Paint of the current PainterState. */
struct FillPaintCommand : public Command {
    FillPaintCommand(Paint paint)
        : Command(SET_FILL_PAINT), paint(paint) {}
    Paint paint;
};

/** Command to set the stroke Color of the current PainterState. */
struct StrokeColorCommand : public Command {
    StrokeColorCommand(Color color)
        : Command(SET_STROKE_COLOR), color(color) {}
    Color color;
};

/** Command to set the stroke Paint of the current PainterState. */
struct StrokePaintCommand : public Command {
    StrokePaintCommand(Paint paint)
        : Command(SET_STROKE_PAINT), paint(paint) {}
    Paint paint;
};

/** Command to set the stroke width of the current PainterState. */
struct StrokeWidthCommand : public Command {
    StrokeWidthCommand(float stroke_width)
        : Command(SET_STROKE_WIDTH), stroke_width(stroke_width) {}
    float stroke_width;
};

/** Command to set the BlendMode of the current PainterState. */
struct BlendModeCommand : public Command {
    BlendModeCommand(BlendMode blend_mode)
        : Command(SET_BLEND_MODE), blend_mode(blend_mode) {}
    BlendMode blend_mode;
};

/** Command to set the alpha of the current PainterState. */
struct SetAlphaCommand : public Command {
    SetAlphaCommand(float alpha)
        : Command(SET_ALPHA), alpha(alpha) {}
    float alpha;
};

/** Command to set the MiterLimit of the current PainterState. */
struct MiterLimitCommand : public Command {
    MiterLimitCommand(float miter_limit)
        : Command(SET_MITER_LIMIT), miter_limit(miter_limit) {}
    float miter_limit;
};

/** Command to set the LineCap of the current PainterState. */
struct LineCapCommand : public Command {
    LineCapCommand(Painter::LineCap line_cap)
        : Command(SET_LINE_CAP), line_cap(line_cap) {}
    Painter::LineCap line_cap;
};

/** Command to set the LineJoinof the current PainterState. */
struct LineJoinCommand : public Command {
    LineJoinCommand(Painter::LineJoin line_join)
        : Command(SET_LINE_JOIN), line_join(line_join) {}
    Painter::LineJoin line_join;
};

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

        Path(std::vector<Point>& points)
            : first_point(points.size())
            , point_count(0)
            , winding(Painter::Winding::COUNTERCLOCKWISE)
            , is_closed(false)
            , is_convex(true)
        {
        }

        /** Index of the first Point. */
        size_t first_point;

        /** Number of Points in this Path. */
        size_t point_count;

        /** What direction the Path is wound. */
        Painter::Winding winding;

        /** Whether this Path is closed or not.
         * Closed Paths will draw an additional line between their last and first Point.
         */
        bool is_closed;

        /** Whether the Path is convex or not. */
        bool is_convex;
    };

public: // methods
    /** Resets the Painterpreter to its original, empty state. */
    void reset();

    float get_distance_tolerance() const { return m_distance_tolerance; }
    float get_tesselation_tolerance() const { return m_tesselation_tolerance; }
    float get_fringe_width() const { return m_fringe_width; }

    void set_distance_tolerance(float tolerance) { m_distance_tolerance = tolerance; }

    void set_tesselation_tolerance(float tolerance) { m_tesselation_tolerance = tolerance; }

    void set_fringe_width(float width) { m_fringe_width = width; }

    /** Adds a new Command to be executed in order. */
    template <typename Subcommand, ENABLE_IF_SUBCLASS(Subcommand, Command)>
    inline void add_command(Subcommand command)
    {
        using array_t      = std::array<Command::command_t, sizeof(command) / sizeof(Command::command_t)>;
        array_t& raw_array = *(reinterpret_cast<array_t*>(&command));
        m_commands.insert(std::end(m_commands),
                          std::make_move_iterator(std::begin(raw_array)),
                          std::make_move_iterator(std::end(raw_array)));
    }

    void _push_state();

    void _pop_state();

    /** Appends a new Point to the current Path. */
    void add_point(Vector2f position, const Point::Flags flags);

    /** Creates a new, empty Path. */
    void add_path() { m_paths.emplace_back(m_points); }

    /** Calculates the area of a given Path. */
    float get_path_area(const Path& path) const;

    /** Tesellates a Bezier curve.
     * Note that I am using the (experimental) improvement on the standard nanovg tesselation algorithm here,
     * as found at: https://github.com/memononen/nanovg/issues/328
     * If there seems to be an issue with the tesselation, revert back to the "official" implementation.
     */
    void tesselate_bezier(const float x1, const float y1, const float x2, const float y2,
                          const float x3, const float y3, const float x4, const float y4);

    /** Chooses whether to bevel a joint or not and returns vertex coordinates. */
    std::tuple<float, float, float, float>
    choose_bevel(bool is_beveling, const Point& prev_point, const Point& curr_point, const float stroke_width);

    void create_bevel_join(const Point& previous_point, const Point& current_point, const float left_w, const float right_w,
                           const float left_u, const float right_u, std::vector<Vertex>& vertices_out);

    void create_round_join(const Point& previous_point, const Point& current_point, const float stroke_width,
                           const size_t divisions, std::vector<Vertex>& vertices_out);

    /** Creates the round cap and the start of a line.
     * @param point         Start point of the line.
     * @param delta         Direction of the cap.
     * @param stroke_width  Width of the base of the cap.
     * @param divisions     Number of divisions for the cap (per half-circle).
     * @param vertices_out  [out] Vertex vector to write the cap into.
     */
    void create_round_cap_start(const Point& point, const Vector2f& delta, const float stroke_width,
                                const size_t divisions, std::vector<Vertex>& vertices_out);

    /** Creates the round cap and the end of a line.
     * @param point         End point of the line.
     * @param delta         Direction of the cap.
     * @param stroke_width  Width of the base of the cap.
     * @param divisions     Number of divisions for the cap (per half-circle).
     * @param vertices_out  [out] Vertex vector to write the cap into.
     */
    void create_round_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                              const size_t divisions, std::vector<Vertex>& vertices_out);

    void create_butt_cap_start(const Point& point, const Vector2f& direction, const float stroke_width,
                               const float d, const float fringe_width, std::vector<Vertex>& vertices_out);

    void create_butt_cap_end(const Point& point, const Vector2f& delta, const float stroke_width,
                             const float d, const float fringe_width, std::vector<Vertex>& vertices_out);

    void _fill(Cell& cell, const RenderContext& context);
    void _stroke(Cell& cell, const RenderContext& context);
    void _prepare_paths(Cell& cell, const float fringe, const Painter::LineJoin join, const float miter_limit);
    void _execute(Cell& cell, const RenderContext& context);

private: // methods
    /** The current State. */
    PainterState& _get_current_state()
    {
        assert(!m_states.empty());
        return m_states.back();
    }

public: // fields
    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<Command::command_t> m_commands;

    /** Points making up the Painter Paths. */
    std::vector<Point> m_points;

    /** Intermediate representation of the Painter Paths. */
    std::vector<Path> m_paths;

    /** Stack of painter states. */
    std::vector<PainterState> m_states;

    // TODO: all of the fields below are dependent on the RenderContext, yet the context is a paramter for some functions and not for others...

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float m_distance_tolerance;

    /** Tesselation density when creating rounded shapes. */
    float m_tesselation_tolerance;

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float m_fringe_width;
};

} // namespace notf
