#pragma once

#include "common/vector2.hpp"

namespace notf {

/** A 2D Vertex with position and uv-coordinates, can be used in an OpenGL buffer. */
struct Vertex {

    /** Position of the Vertex. */
    Vector2 pos;

    /** UV-coordinates of the Vertex. */
    Vector2 uv;

};

} // namespace notf
