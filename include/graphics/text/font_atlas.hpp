#pragma once

#include <cstdint>
#include <memory>
#include <stddef.h>
#include <vector>

#include "common/forwards.hpp"
#include "graphics/core/gl_forwards.hpp"
#include "graphics/text/font.hpp"

namespace notf {

/// @brief A texture atlas is a texture that is filled with Glyphs.
///
/// Internally, it uses the SKYLINE-BL-WM-BNF pack algorithm as described in:
///     http://clb.demon.fi/projects/more-rectangle-bin-packing
/// and code adapted from:
///     http://clb.demon.fi/files/RectangleBinPack/
///
/// Does not rotate the Glyphs, because I assume that the added complexity and overhead is not worth the trouble.
/// However, the reference implementation above does include rotation, so if you want, implement it and see if it makes
/// a difference.
///
/// Note that in the Atlas, just as with any other OpenGL texture, y grows up.
class FontAtlas {

    using coord_t = Glyph::coord_t;

    using area_t = Glyph::area_t;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using ProtoGlyph = std::pair<codepoint_t, Glyph::Rect>;

    struct FitRequest {
        /// @brief Codepoint, used to associate the answer to the request.
        codepoint_t code_point;

        /// @brief Width of the rectangle to fit into the atlas.
        coord_t width;

        /// @brief Height of the rectangle to fit into the atlas.
        coord_t height;
    };

private:
    /// @brief Helper data structure to keep track of the free space of the bin where rectangles may be placed.
    class WasteMap {

    public: // methods
        /// @brief Constructor.
        WasteMap() = default;

        /// @brief (Re-)initializes the WasteMap.
        void initialize(const coord_t width, const coord_t height);

        /// @brief Registers a new rectangle as "waste".
        void add_waste(Glyph::Rect rect);

        /// @brief Tries to reclaim a rectangle of the given size from waste.
        /// The returned rectangle has zero width and height if reclaiming failed.
        Glyph::Rect reclaim_rect(const coord_t width, const coord_t height);

    private: // fields
        /// @brief Disjoint rectangles of free space that are located below the skyline in the Atlas.
        std::vector<Glyph::Rect> m_free_rects;
    };

    /// @brief A single node (a horizontal line) of the skyline envelope
    struct SkylineNode {
        /// @brief Horizontal start of the line.
        coord_t x;

        /// @brief Heigh of the line.
        coord_t y;

        /// @brief Width of the line from x going right.
        coord_t width;
    };

    /// @brief Return value of '_place_rect'.
    struct ScoredRect {
        /// @brief Rectangular area of the Atlas.
        Glyph::Rect rect;

        /// @brief Index of the best node to insert rect, is max(size_t) if invalid.
        size_t node_index;

        /// @brief Width of the space used when inserting rect at the best position.
        coord_t node_width;

        /// @brief Height of the space used when inserting rect at the best position.
        coord_t new_height;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(FontAtlas)

    /// @brief Default constructor.
    FontAtlas(GraphicsContext& graphics_context);

    /// @brief Destructor.
    ~FontAtlas();

    /// @brief Resets the Texture Atlas without changing its size.
    void reset();

    /// @brief Computes the ratio of used atlas area, returns value in range [0, 1].
    float get_occupancy() const { return static_cast<float>(m_used_area) / (m_width * m_height); }

    /// @brief The Atlas Texture.
    TexturePtr get_texture() const { return m_texture; }

    /// @brief Places and returns a single rectangle into the Atlas.
    /// If you want to insert multiple known rects, calling `insert_rects` will likely yield a tighter result than
    /// multiple calls to `insert_rect`.
    Glyph::Rect insert_rect(const coord_t width, const coord_t height);

    /// @brief Places and returns multiple rectangles into the Atlas.
    /// Produces a better fit than multiple calls to `insert_rect`.
    std::vector<ProtoGlyph> insert_rects(std::vector<FitRequest> named_extends);

    /// @brief Fills a rect in the Atlas with the given data.
    /// Does not check whether the rect corresponds to a node in the atlas, I trust you know what you are doing.
    void fill_rect(const Glyph::Rect& rect, const uchar* data);

private:
    /// @brief Finds and returns a free rectangle in the Atlas of the requested size
    /// Also returns information about the generated waste allowing the 'insert' functions to optimize the order in
    /// which new Glyphs are created.
    ScoredRect _get_rect(const coord_t width, const coord_t height) const;

    /// @brief Creates a new Skyline node just left of the given node index.
    void _add_node(const size_t node_index, const Glyph::Rect& rect);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Graphics context owning the texture.
    GraphicsContext& m_graphics_context;

    /// @brief Font atlas texture.
    TexturePtr m_texture;

    /// @brief Width of the texture atlas.
    coord_t m_width;

    /// @brief Height of the texture atlas.
    coord_t m_height;

    /// @brief Used surface area in this atlas.
    area_t m_used_area;

    /// @brief All nodes of the atlas, used to find free space for new glyphs.
    std::vector<SkylineNode> m_nodes;

    /// @brief Separate data structure to keep track of waste undereath the skyline.
    WasteMap m_waste;
};

} // namespace notf
