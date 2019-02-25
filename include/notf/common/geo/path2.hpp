#pragma once

#include "notf/meta/hash.hpp"

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/matrix3.hpp"
#include "notf/common/geo/size2.hpp"
#include "notf/common/vector.hpp"

NOTF_OPEN_NAMESPACE

/// A Path is a unified representation of a 2D line.
/// Contains of sub-paths.
/// Each sub-path can be closed or open, going clock- or counterclockwise.
/// Internally, a Path is always stored as a cubic bezier spline, even if it only consists of straight lines.
/// A path is normalized so that two paths can be compared equal, even if they are rotated or mirrored.
/// They are not optimized for fast computation (like line intersection etc.) but serve as an interface to user
/// constructed lines and shapes.
/// Paths can be created from other geometrical constructs like circles, rects or polygons, or via the "PathPainter" (or
/// whatever) that behaves like an HTML5 canvas painter.
/// Are immutable.
/// Are usually stored alongside a 2D transformation.
class Path2 {

    template<class>
    friend struct std::hash;

    // types ----------------------------------------------------------------------------------- //
private:
    struct Path {
        struct Vertex {
            Vertex(V2d pos) : pos(pos) {}

            V2d pos;
            V2d left_tangent = V2d::zero();
            V2d right_tangent = V2d::zero();
            double left_distance = 0;
            double right_distance = 0;
        };

        struct Subpath {
            size_t first_index;
            size_t size;
            bool is_closed = true;
            bool is_convex = true;
        };

        void add_subpath(std::initializer_list<Vertex>&& vertices);
        template<class... Ts>
        void add_subpath(Ts&&... args) {
            add_subpath({std::forward<Ts>(args)...});
        }

        bool is_empty() { return m_vertices.empty(); }

        size_t get_hash();

        Aabrd get_aabr() const {
            Aabrd result = Aabrd::wrongest();
            for (const Vertex& vertex : m_vertices) {
                result.grow_to(vertex.pos);
                result.grow_to(vertex.pos + vertex.left_tangent * vertex.left_distance);
                result.grow_to(vertex.pos + vertex.right_tangent * vertex.right_distance);
            }
            return result;
        }

    private:
        std::vector<Vertex> m_vertices;
        std::vector<Subpath> m_subpaths;
    };
    using PathPtr = std::shared_ptr<Path>;

public:
    /// Winding direction of subpath.
    enum class Winding {
        CCW,
        CW,
        COUNTERCLOCKWISE = CCW,
        CLOCKWISE = CW,
        SOLID = CCW,
        HOLE = CW,
    };

    //    class Drawer {

    //        // Moves the starting point of a new sub-path to the (x, y) coordinates.
    //        void start_path();

    //        // Causes the point of the pen to move back to the start of the current sub-path. It tries to draw a
    //        straight
    //        // line from the current point to the start. If the shape has already been closed or has only one point,
    //        this
    //        // function does nothing.
    //        void close_path();

    //        // Sets the winding of the current sub-path.
    //        void set_winding();

    //        // Connects the last point in the subpath to the (x, y) coordinates with a straight line.
    //        void line_to();

    //        // Adds a cubic Bézier curve to the path. It requires three points. The first two points are control
    //        points and
    //        // the third one is the end point. The starting point is the last point in the current path, which can be
    //        // changed using moveTo() before creating the Bézier curve.
    //        void bezier_curve_to();

    //        // Adds a quadratic Bézier curve to the current path.
    //        void quadratic_curve_to();

    //        // Adds a circular arc to the path with the given control points and radius, connected to the previous
    //        point by
    //        // a straight line.
    //        // Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
    //        // and the arc is drawn from angle a0 to a1, and swept in direction dir (NVG_CCW, or NVG_CW).
    //        // Angles are specified in radians.
    //        void arc_to();
    //    };

    // methods --------------------------------------------------------------------------------- //
private:
    Path2(PathPtr path) : m_path(std::move(path)) {}

public:
    // Path2D constructor. Creates a new Path2D object.
    Path2() = default;

    static Path2 rect();

    bool is_empty() const { return m_path->is_empty(); }

    const M3d& get_xform() const { return m_xform; }

    Aabrd get_aabr() const { return m_path->get_aabr(); }

    Size2d get_size() const { return m_path->get_aabr().get_size(); }

    bool operator==(const Path2& other) { return other.m_path == m_path && other.m_xform == m_xform; }
    bool operator!=(const Path2& other) { return other.m_path != m_path || other.m_xform != m_xform; }

    //    // Creates new rectangle shaped sub-path.
    //    void nvgRect(NVGcontext* ctx, float x, float y, float w, float h);

    //    // Creates new rounded rectangle shaped sub-path.
    //    void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r);

    //    // Creates new rounded rectangle shaped sub-path with varying radii for each corner.
    //    void nvgRoundedRectVarying(NVGcontext* ctx, float x, float y, float w, float h, float radTopLeft, float
    //    radTopRight, float radBottomRight, float radBottomLeft);

    //    // Creates new ellipse shaped sub-path.
    //    void nvgEllipse(NVGcontext* ctx, float cx, float cy, float rx, float ry);

    //    // Creates new circle shaped sub-path.
    //    void nvgCircle(NVGcontext* ctx, float cx, float cy, float r);

    // fields ---------------------------------------------------------------------------------- //
private:
    PathPtr m_path;

    M3d m_xform = M3d::identity();
};

NOTF_CLOSE_NAMESPACE
