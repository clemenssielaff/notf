#pragma once

#include "common/aabr.hpp"
#include "graphics/cell/command_buffer.hpp"
#include "graphics/vertex.hpp"

namespace notf {

class Cell {

public:
    friend class Painter;

    struct Path {
        size_t point_offset; // points
        size_t point_count;
        bool is_closed;
        size_t fill_offset; // vertices
        size_t fill_count;
        size_t stroke_offset; // vertices
        size_t stroke_count;
        unsigned char winding;
        bool is_convex;

        Path(size_t first)
            : point_offset(first)
            , point_count(0)
            , is_closed(false)
            , fill_offset(0)
            , fill_count(0)
            , stroke_offset(0)
            , stroke_count(0)
            , winding(0)
            , is_convex(false)
        {
        }
    };

    struct Point {
        enum Flags : unsigned char {
            NONE       = 0,
            CORNER     = 1u << 1,
            LEFT       = 1u << 2,
            BEVEL      = 1u << 3,
            INNERBEVEL = 1u << 4,
        };

        /** Position of the Point. */
        Vector2f pos;

        /** Direction to the next Point. */
        Vector2f forward;

        /** Something with miter...? */
        Vector2f dm;

        /** Distance to the next point forward. */
        float length;

        /** Additional information about this Point. */
        Flags flags;
    };

    //public: // methods
    Cell() = default;

    /** The Painter Command buffer of this Cell. */
    const PainterCommandBuffer& get_commands() const { return m_commands; }

    //private: // fields
    PainterCommandBuffer m_commands;

    std::vector<Point> m_points;

    std::vector<Path> m_paths;

    std::vector<Vertex> m_vertices;

    /** The bounding rectangle of the Cell. */
    Aabrf m_bounds;
};

} // namespace notf
