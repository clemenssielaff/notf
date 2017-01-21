#pragma once

namespace notf {

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

struct Path {
    int first;
    int count;
    unsigned char closed;
    int nbevel;
    Vertex* fill;
    int nfill;
    Vertex* stroke;
    int nstroke;
    int winding;
    int convex;
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
};

} // namespace notf
