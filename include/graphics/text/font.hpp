#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/hash.hpp"
#include "common/meta.hpp"
#include "common/utf.hpp"

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace notf {

class FontManager;
class GraphicsContext;

DEFINE_SHARED_POINTERS(class, Font);

/// @brief Data type to identify a single Glyph.
using codepoint_t = utf32_t;

//====================================================================================================================//

/// @brief A Glyph contains information about how to render a single character from a font atlas.
/// Glyph coordinates are stored as signed integers, because they can be negative as well.
///
///          ^
///          |
///          |                <-rect.width->
///          |          +--------------------------+
///          |          |        :xKWMMWKd,        |
///          |          |    c0MMMKxodOWMMW0OOOOOOO|
///          |          |  ;WMMMO.      dMMMMWdoooo|     ^
///          |     ^    | ;MMMM0         xMMMMo    |     |
///          |     |    | 0MMMMl         'MMMMW    |     |
///          |    top   | OMMMMd         :MMMMN    |     |
///          |     |    | .WMMMW.       .XMMMW;    |     |
///          |          |  .OMMMWd'   .lNMMMk.     |  rect.height
///          |          |    .ckXMMMMMMN0x:        |     |
///          |          |      'XMK..              |     |
///          |  -left-> |     :MMMk.               |     |
///        --X----------|-----0MMMMMMMWNKOdl,.-----|-----|--x--->
///   origin |          |    'kMMNKXWMMMMMMMMMXo   |     |  |
///          |          | ,0MMWo.      .,cxNMMMMN. |     v
///          |          |oMMMM.             oMMMMd |        |
///          |          |WMMMW              .MMMM' |
///          |          |XMMMMd            .KMMW;  |        |
///          |          |.XMMMMK:.       'dWMNo    |
///          |          |  ;OWMMMMN0OOOXMW0o'      |        |
///          |          |     ;xKWMMMN0d,          |
///          |          +--------------------------+        |
///          |
///          |- - - - - - - - - - - - - - - - - - - - - - - +
///          |                 -advance_x->
///          v
///
struct Glyph {

    /// @brief Integer type to store a single Glyph coordinate.
    using coord_t = int16_t;

    /// @brief Integer type that can be used to express an area (coordinate_type^2).
    using area_t = int32_t;

    /// @brief Rectangular area inside the Atlas.
    struct Rect {
        /// @brief X-coordinate of the rectangle in the atlas.
        coord_t x;

        /// @brief Y-coordinate of the rectangle in the atlas.
        coord_t y;

        /// @brief Width of the rectangle in pixels.
        coord_t width;

        /// @brief Height of the rectangle in pixels.
        coord_t height;

        /// @brief Default Constructor.
        Rect() = default;

        /// @brief Value Constructor.
        Rect(coord_t x, coord_t y, coord_t width, coord_t height) : x(x), y(y), width(width), height(height) {}
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Rectangle of the FontAtlas that contains the texture of this Glyph.
    Rect rect;

    /// @brief Distance to the left side of the Glyph from the origin in pixels.
    coord_t left;

    /// @brief Distance to the top of the Glyph from the baseline in pixels.
    coord_t top;

    /// @brief How far to advance the origin horizontal.
    coord_t advance_x;

    /// @brief How far to advance the origin vertically.
    coord_t advance_y;
};

//====================================================================================================================//

/// @brief A Font is a manger object for a given font face in the RenderManager.
/// It knows where its Glyphs reside in the RenderManager's Font Atlas.
/// At the moment, a Font renders all renderable ascii Glyphs to begin with and adds new Glyphs should they be
/// requested.
class Font {

    friend class FontManager;

public: // types
    using pixel_size_t = ushort;

    /// @brief Every Font is uniquely identified by its file and the Font size in pixels.
    struct Identifier {

        /// @brief Filename of the loaded Font.
        std::string filename;

        /// @brief Pixel size of this Font.
        pixel_size_t pixel_size;

        /// @brief Equality test operator. f*/
        bool operator==(const Identifier& rhs) const
        {
            return filename == rhs.filename && pixel_size == rhs.pixel_size;
        }
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// @brief Constructor.
    /// @param manager       Font manager.
    /// @param filename      File from which to load the font.
    /// @param pixel_size    Font base size in pixels.
    Font(FontManager& manager, const std::string filename, const pixel_size_t pixel_size);

public:
    /// @brief Loads a new Font or returns a pointer to an existing font if a font with the same filename /
    /// pixel_size-pair has already been loaded.
    /// @param context       Render Context in which the Font lives.
    /// @param filename      File from which the Font is loaded.
    /// @param pixel_size    Nominal size of the loaded Font in pixels.
    static std::shared_ptr<Font>
    load(GraphicsContext& context, const std::string filename, const pixel_size_t pixel_size);

    /// @brief Returns true if this Font is valid.
    /// If the file used to initialize the Font could not be loaded, the Font is invalid.
    bool is_valid() const { return m_face; }

    /// @brief Returns the requested Glyph, or an invalid Glyph if the given codepoint is not known.
    const Glyph& glyph(const codepoint_t codepoint) const;

    /// @brief Font base size in pixels.
    pixel_size_t pixel_size() const { return m_identifier.pixel_size; }

    /// @brief The vertical distance from the horizontal baseline to the hightest ‘character’ coordinate in a font face.
    pixel_size_t ascender() { return m_ascender; }

    /// @brief The vertical distance from the horizontal baseline to the lowest ‘character’ coordinate in a font face.
    pixel_size_t descender() { return m_descender; }

    /// @brief Default vertical baseline-to-baseline distance.
    pixel_size_t line_height() { return m_line_height; }

private:
    /// @brief Renders and returns a new Glyph.
    const Glyph& _allocate_glyph(const codepoint_t codepoint) const;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Font Manager.
    FontManager& m_manager;

    /// @brief Name of the Font.
    std::string m_name;

    /// @brief Every Font is uniquely identified by its file and the Font size in pixels.
    const Identifier m_identifier;

    /// @brief Freetype font face.
    FT_Face m_face;

    /// @brief The vertical distance from the horizontal baseline to the hightest ‘character’ coordinate in a font face.
    pixel_size_t m_ascender;

    /// @brief The vertical distance from the horizontal baseline to the lowest ‘character’ coordinate in a font face.
    pixel_size_t m_descender;

    /// @brief Default vertical baseline-to-baseline distance, usually larger than the sum of the ascender and
    /// descender. There is no guarantee that no glyphs extend above or below subsequent baselines when using this
    /// height.
    pixel_size_t m_line_height;

    /// @brief Glyphs indixed by code point.
    mutable std::unordered_map<codepoint_t, Glyph> m_glyphs;
};

} // namespace notf

//== std::hash =======================================================================================================//

namespace std {

/// @brief std::hash specialization for Font::Identifier.
template<>
struct hash<notf::Font::Identifier> {
    size_t operator()(const notf::Font::Identifier& id) const { return notf::hash(id.filename, id.pixel_size); }
};

} // namespace std
