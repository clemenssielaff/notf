#pragma once

#include "common/aabr.hpp"
#include "graphics/cell/paint.hpp"
#include "graphics/scissor.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class Cell {

    friend class Painter;
    friend class RenderContext;
    friend class Painterpreter;

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
        Scissor scissor;
        float stroke_width = 0;
    };

public: // methods
    /** A minimal bounding rect containing all Vertices in the Cell. */
    const Aabrf& get_bounds() const { return m_bounds; }

private: // fields
    Aabrf m_bounds;
    std::vector<Call> m_calls;
    std::vector<Path> m_paths;
    std::vector<Vertex> m_vertices;
};

} // namespace notf
