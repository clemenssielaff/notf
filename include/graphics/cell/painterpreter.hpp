#pragma once

#include <array>

#include "common/vector2.hpp"
#include "graphics/cell/painter.hpp"
#include "utils/enum_to_number.hpp"

namespace notf {

class Cell;
struct Vertex;

/**********************************************************************************************************************/

struct Command {
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
        : type(static_cast<float>(to_number(type))) {}

public: // field
    const float type;
};
static_assert(sizeof(Command::Type) == sizeof(float),
              "Floats on your system don't seem be to be 32 bits wide. "
              "Adjust the type of the underlying type of Painter::Command to fit your particular system.");
static_assert(sizeof(Command) == sizeof(float) * 1, "Compiler added padding to the BezierCommand struct");

struct PushStateCommand : public Command {
    PushStateCommand()
        : Command(PUSH_STATE) {}
};
static_assert(sizeof(PushStateCommand) == sizeof(float) * 1, "Compiler added padding to the PushStateCommand struct");

struct PopStateCommand : public Command {
    PopStateCommand()
        : Command(POP_STATE) {}
};
static_assert(sizeof(PopStateCommand) == sizeof(float) * 1, "Compiler added padding to the PopStateCommand struct");

struct BeginCommand : public Command {
    BeginCommand()
        : Command(BEGIN_PATH) {}
};
static_assert(sizeof(BeginCommand) == sizeof(float) * 1, "Compiler added padding to the BeginPathCommand struct");

struct SetWindingCommand : public Command {
    SetWindingCommand(Painter::Winding winding)
        : Command(SET_WINDING), winding(static_cast<float>(to_number(winding))) {}
    float winding;
};
static_assert(sizeof(SetWindingCommand) == sizeof(float) * 2, "Compiler added padding to the SetWindingCommand struct");

struct CloseCommand : public Command {
    CloseCommand()
        : Command(CLOSE) {}
};
static_assert(sizeof(CloseCommand) == sizeof(float) * 1, "Compiler added padding to the ClosePathCommand struct");

struct MoveCommand : public Command {
    MoveCommand(Vector2f pos)
        : Command(MOVE), pos(std::move(pos)) {}
    Vector2f pos;
};
static_assert(sizeof(MoveCommand) == sizeof(float) * 3, "Compiler added padding to the MoveCommand struct");

struct LineCommand : public Command {
    LineCommand(Vector2f pos)
        : Command(LINE), pos(std::move(pos)) {}
    Vector2f pos;
};
static_assert(sizeof(LineCommand) == sizeof(float) * 3, "Compiler added padding to the LineCommand struct");

struct BezierCommand : public Command {
    BezierCommand(Vector2f ctrl1, Vector2f ctrl2, Vector2f end)
        : Command(BEZIER), ctrl1(std::move(ctrl1)), ctrl2(std::move(ctrl2)), end(std::move(end)) {}
    Vector2f ctrl1;
    Vector2f ctrl2;
    Vector2f end;
};
static_assert(sizeof(BezierCommand) == sizeof(float) * 7, "Compiler added padding to the BezierCommand struct");

struct FillCommand : public Command {
    FillCommand()
        : Command(FILL) {}
};
static_assert(sizeof(FillCommand) == sizeof(float) * 1, "Compiler added padding to the FillCommand struct");

struct StrokeCommand : public Command {
    StrokeCommand()
        : Command(STROKE) {}
};
static_assert(sizeof(StrokeCommand) == sizeof(float) * 1, "Compiler added padding to the StrokeCommand struct");

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

    bool is_current_path_empty() const { return m_paths.empty() || m_paths.back().point_count == 0; }

    /** Adds a new Command to be executed in order. */
    template <typename Subcommand, ENABLE_IF_SUBCLASS(Subcommand, Command)>
    inline void add_command(Subcommand command)
    {
        using array_t        = std::array<float, sizeof(command) / sizeof(float)>;
        array_t& float_array = *(reinterpret_cast<array_t*>(&command));
        m_commands.insert(std::end(m_commands),
                          std::make_move_iterator(std::begin(float_array)),
                          std::make_move_iterator(std::end(float_array)));
    }

    /** Appends a new Point to the current Path. */
    void add_point(const Vector2f position, const Point::Flags flags);

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

    void _fill(Cell& cell);
    void _stroke(Cell& cell);
    void _prepare_paths(Cell& cell, const float fringe, const Painter::LineJoin join, const float miter_limit);
    void _execute(Cell& cell);

public: // fields
    /** Bytecode-like instructions, separated by COMMAND values. */
    std::vector<float> m_commands;

    /** Points making up the Painter Paths. */
    std::vector<Point> m_points;

    /** Intermediate representation of the Painter Paths. */
    std::vector<Path> m_paths;

    /** Furthest distance between two points in which the second point is considered equal to the first. */
    float m_distance_tolerance;

    /** Tesselation density when creating rounded shapes. */
    float m_tesselation_tolerance;

    /** Width of the faint outline around shapes when geometric antialiasing is enabled. */
    float m_fringe_width;
};

} // namespace notf
