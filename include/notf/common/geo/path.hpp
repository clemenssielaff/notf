/* A Path is a unified representation of a 2D line.
 * It can be closed or open, going clock- or counterclockwise.
 * Internally, a Path is always stored as a cubic bezier spline, even if it only consists of straight lines.
 * A path is normalized and stores an additional transformation, so that two paths can be compared equal, even if they
 * are rotated or mirrored against each other.
 * They are not optimized for fast computation (like line intersection etc.) but serve as an interface to user
 * constructed lines and shapes.
 * Paths can be created from other geometrical constructs like circles, rects or polygons, or via the "PathPainter" (or
 * whatever) that behaves like an HTML5 canvas painter.
 */

#include "notf/common/vector2.hpp"

NOTF_OPEN_NAMESPACE

class Path2D {

    struct Vertex {
        V2d left_ctrl;
        V2d pos;
        V2d right_ctrl;
    };

    struct SubPath {
        size_t first_index;
        size_t last_index;
    };

    class Painter {

        // Causes the point of the pen to move back to the start of the current sub-path. It tries to draw a straight
        // line from the current point to the start. If the shape has already been closed or has only one point, this
        // function does nothing.
        void closePath();

        // Moves the starting point of a new sub-path to the (x, y) coordinates.
        void moveTo();

        // Connects the last point in the subpath to the (x, y) coordinates with a straight line.
        void lineTo();

        // Adds a cubic Bézier curve to the path. It requires three points. The first two points are control points and
        // the third one is the end point. The starting point is the last point in the current path, which can be
        // changed using moveTo() before creating the Bézier curve.
        void bezierCurveTo();

        // Adds a quadratic Bézier curve to the current path.
        void quadraticCurveTo();

        // Adds a circular arc to the path with the given control points and radius, connected to the previous point by
        // a straight line.
        void arcTo();
    };

    // Path2D constructor. Creates a new Path2D object.
    Path2D();

    // Adds a path to the current path.
    void addPath();

//    // Sets the current sub-path winding, see NVGwinding and NVGsolidity.
//    void nvgPathWinding(NVGcontext* ctx, int dir);

//    // Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
//    // and the arc is drawn from angle a0 to a1, and swept in direction dir (NVG_CCW, or NVG_CW).
//    // Angles are specified in radians.
//    void nvgArc(NVGcontext* ctx, float cx, float cy, float r, float a0, float a1, int dir);

//    // Creates new rectangle shaped sub-path.
//    void nvgRect(NVGcontext* ctx, float x, float y, float w, float h);

//    // Creates new rounded rectangle shaped sub-path.
//    void nvgRoundedRect(NVGcontext* ctx, float x, float y, float w, float h, float r);

//    // Creates new rounded rectangle shaped sub-path with varying radii for each corner.
//    void nvgRoundedRectVarying(NVGcontext* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

//    // Creates new ellipse shaped sub-path.
//    void nvgEllipse(NVGcontext* ctx, float cx, float cy, float rx, float ry);

//    // Creates new circle shaped sub-path.
//    void nvgCircle(NVGcontext* ctx, float cx, float cy, float r);
};

NOTF_CLOSE_NAMESPACE
