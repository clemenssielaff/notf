#pragma once

#include "graphics/paint.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class Cell {

    friend class Painter;

private: // types
    struct Path {
        size_t fill_offset   = 0;
        size_t fill_count    = 0;
        size_t stroke_offset = 0;
        size_t stroke_count  = 0;
    };

    struct Call {
        enum class Type : unsigned char {
            FILL,
            CONVEX_FILL,
            STROKE,
        };
        Type type          = Type::FILL;
        size_t path_offset = 0;
        size_t path_count  = 0;
        Paint paint;
    };

private: // fields
    std::vector<Call> m_calls;
    std::vector<Path> m_paths;
    std::vector<Vertex> m_vertices;
};

} // namespace notf
