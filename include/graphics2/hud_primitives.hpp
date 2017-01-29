#pragma once

#include <vector>

#include "common/color.hpp"
#include "common/size2f.hpp"
#include "common/transform2.hpp"

namespace notf {

enum class LineCap : unsigned char {
    BUTT,
    ROUND,
    SQUARE,
};

enum class LineJoin : unsigned char {
    MITER,
    ROUND,
    BEVEL,
};

enum class Winding : unsigned char {
    CCW,
    CW,
    SOLID = CCW,
    HOLES = CW,
};

struct Point {
    float x,y;
    float dx, dy;
    float len;
    float dmx, dmy;
    unsigned char flags;
};

struct Vertex {
    float x,y,u,v;
};

// TODO: hud namespace for hud-related classes

struct Path {
    int first;
    int count;
    unsigned char closed;
    int nbevel;
    std::vector<Vertex> fill;
    std::vector<Vertex> stroke;
    Winding winding;
    bool is_convex;
};

struct PathCache {
    Point* points;
    int npoints;
    int cpoints;
    Path* paths;
    int npaths;
    int cpaths;
    Vertex* verts;
    int nverts;
    int cverts;
    float bounds[4];

    void clear()
    {
        npoints = npaths = 0;
    }
};

struct BlendMode {

    // In nanovg, you can set the source- and destionation factor independently
    // but you cannot set the operation function
    // ... for now, I'd settle for the HTML5 canvas modes (without "darken")

    enum Mode : unsigned char {
        SOURCE_OVER,
        SOURCE_IN,
        SOURCE_OUT,
        SOURCE_ATOP,
        DESTINATION_OVER,
        DESTINATION_IN,
        DESTINATION_OUT,
        DESTINATION_ATOP,
        LIGHTER,
        COPY,
        XOR,
    };

    BlendMode()
        : rgb(SOURCE_OVER)
        , alpha(SOURCE_OVER)
    {
    }

    BlendMode(const Mode mode)
        : rgb(mode)
        , alpha(std::move(mode))
    {
    }

    BlendMode(const Mode color, const Mode alpha)
        : rgb(std::move(color))
        , alpha(std::move(alpha))
    {
    }

    Mode rgb;
    Mode alpha;
};

struct Paint {
    Paint() = default;

    Paint(Color color)
        : xform(Transform2::identity())
        , extent()
        , radius(0)
        , feather(1)
        , innerColor(std::move(color))
        , outerColor(innerColor)
        , image(0)
    {
    }

    void set_color(const Color color)
    {
        innerColor = std::move(color);
        outerColor = innerColor;
    }

    Transform2 xform;
    Size2f extent;
    float radius;
    float feather;
    Color innerColor;
    Color outerColor;
    int image;
};

struct Scissor {
    void reset()
    {
        xform  = Transform2::identity();
        extend = {-1, -1};
    }

    /** Scissors have their own transformation. */
    Transform2 xform;

    /** Extend around the center of the Transform.
     * That means that the Scissor's width is extend.width * 2.
     */
    Size2f extend;
};

} // namespace notf
