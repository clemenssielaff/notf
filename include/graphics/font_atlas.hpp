#pragma once

#include <cstdint>
#include <stddef.h>
#include <vector>

#include "graphics/gl_forwards.hpp"

namespace notf {

/** A texture atlas is a texture that is filled with Glyphs.
 *
 * Internally, it uses the SKYLINE-BL-WM-BNF pack algorithm as described in:
 *     http://clb.demon.fi/projects/more-rectangle-bin-packing
 * and code adapted from:
 *     http://clb.demon.fi/files/RectangleBinPack/
 *
 * Does not rotate the Glyphs, because I assume that the added complexity, overhead, branching (and potential OpenGL
 * blurryness?) is not worth the trouble.
 * However, the implementation above does include rotation, so if you want, implement it and see if it makes a
 * difference.
 */
class FontAtlas {

public: // types
    /** Integer type to store a single Glyph coordinate. */
    using coord_t = uint16_t;

    /** Integer type that can be used to express an area (coordinate_type^2). */
    using area_t = uint32_t;

public: // struct
    /** Rectangular area inside the Atlas. */
    struct Rect {
        /** X-coordinate of the rectangle in the atlas. */
        coord_t x;

        /** Y-coordinate of the rectangle in the atlas. */
        coord_t y;

        /** Width of the rectangle in pixels. */
        coord_t width;

        /** Height of the rectangle in pixels. */
        coord_t height;

        /** Default Constructor. */
        Rect() = default;

        /** Value Constructor. */
        Rect(coord_t x, coord_t y, coord_t width, coord_t height)
            : x(x), y(y), width(width), height(height) {}
    };

private: // classes
    /** Helper data structure to keep track of the free space of the bin where rectangles may be placed. */
    class WasteMap {

    public: // methods
        WasteMap() = default;

        /** (Re-)initializes the WasteMap. */
        void initialize(const coord_t width, const coord_t height);

        /** Registers a new rectangle as "waste" */
        void add_waste(Rect rect);

        /** Tries to reclaim a rectangle of the given size from waste.
         * The returned rectangle has zero width and height if reclaiming failed.
         */
        Rect reclaim_rect(const coord_t width, const coord_t height);

    private: // methods
        /** Try to merge waste rectangles. */
        void _consolidate();

    private: // fields
        /** Disjoint rectangles of free space that are located below the skyline in the Atlas. */
        std::vector<Rect> m_waste_rects;
    };

    /** A single level (a horizontal line) of the skyline envelope*/
    struct SkylineNode {
        /** Horizontal start of the line.*/
        coord_t x;

        /** Heigh of the line.*/
        coord_t y;

        /** Width of the line from x going right.*/
        coord_t width;

        /** Default Constructor. */
        SkylineNode() = default;

        /** Value Constructor. */
        SkylineNode(coord_t x, coord_t y, coord_t width)
            : x(x), y(y), width(width) {}
    };

    /** Return value of '_place_rect()'. */
    struct ScoredRect {
        /** Rectangular area of the Atlas. */
        Rect rect;

        /** Index of the best node to insert rect, is max(size_t) if invalid. */
        size_t best_index;

        /** Width of the space used when inserting rect at the best position. */
        coord_t best_width;

        /** Height of the space used when inserting rect at the best position. */
        coord_t best_height;

        ScoredRect() = default;
    };

public: // methods
    /** Default constructor. */
    FontAtlas(const coord_t width = 512, const coord_t height = 512);

    /** Destructor. */
    ~FontAtlas();

    FontAtlas(const FontAtlas&) = delete;            // no copy construction
    FontAtlas& operator=(const FontAtlas&) = delete; // no copy assignment

    /** Resets the Texture Atlas without changing its size. */
    void reset();

    /** Computes the ratio of used atlas area (from 0 -> 1). */
    float get_occupancy() const { return static_cast<float>(m_used_area) / (m_width * m_height); }

    /** Places and returns a single rectangle into the Atlas.
     * If you want to insert multiple known rects, calling `insert_rects` will probably yield a tighter result than
     * multiple calls to `insert_rect`.
     */
    Rect insert_rect(const coord_t width, const coord_t height);

private: // methods
    /** Finds and returns a free rectangle in the Atlas of the requested size
     * Also returns information about the generated waste allowing the 'insert' functions to optimize the order in which
     * new Glyphs are created.
     */
    ScoredRect _get_rect(const coord_t width, const coord_t height) const;

    /** Creates a new Skyline node just left of the given node index. */
    void _add_node(const size_t node_index, const Rect& rect);

private: //
    /** OpenGl texture ID of the Atlas. */
    GLuint m_texture_id;

    /** Width of the texture Atlas. */
    coord_t m_width;

    /** Height of the texture Atlas. */
    coord_t m_height;

    /** Used surface area in this Atlas. */
    area_t m_used_area;

    /** All nodes of the Atlas, used to find free space for new Glyphs. */
    std::vector<SkylineNode> m_nodes;

    /** Separate data structure to keep track of waste undereath the skyline. */
    WasteMap m_waste;
};

} // namespace notf
