#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/hash.hpp"
#include "common/utf.hpp"
#include "graphics/forwards.hpp"

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

NOTF_OPEN_NAMESPACE

class FontManager;
class GraphicsContext;

/// Data type to identify a single Glyph.
using codepoint_t = utf32_t;

//====================================================================================================================//

/// A Glyph contains information about how to render a single character from a font atlas.
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

    /// Integer type to store a single Glyph coordinate.
    using coord_t = int16_t;

    /// Integer type that can be used to express an area (coordinate_type^2).
    using area_t = int32_t;

    /// Rectangular area inside the Atlas.
    struct Rect {
        /// X-coordinate of the rectangle in the atlas.
        coord_t x;

        /// Y-coordinate of the rectangle in the atlas.
        coord_t y;

        /// Width of the rectangle in pixels.
        coord_t width;

        /// Height of the rectangle in pixels.
        coord_t height;

        /// Default Constructor.
        Rect() = default;

        /// Value Constructor.
        Rect(coord_t x, coord_t y, coord_t width, coord_t height) : x(x), y(y), width(width), height(height) {}
    };

    // fields --------------------------------------------------------------------------------------------------------//
    /// Rectangle of the FontAtlas that contains the texture of this Glyph.
    Rect rect;

    /// Distance to the left side of the Glyph from the origin in pixels.
    coord_t left;

    /// Distance to the top of the Glyph from the baseline in pixels.
    coord_t top;

    /// How far to advance the origin horizontal.
    coord_t advance_x;

    /// How far to advance the origin vertically.
    coord_t advance_y;
};

//====================================================================================================================//

/// A Font is a manger object for a given font face in the FontManager.
/// It knows where its Glyphs reside in the FontManager's Font Atlas.
/// At the moment, a Font renders all renderable ascii Glyphs to begin with and adds new Glyphs should they be
/// requested.
class Font {

    friend class FontManager;

public: // types
    using pixel_size_t = ushort;

    /// Every Font is uniquely identified by its file and the Font size in pixels.
    struct Identifier {

        /// Filename of the loaded Font.
        std::string filename;

        /// Pixel size of this Font.
        pixel_size_t pixel_size;

        /// Equality test operator.
        bool operator==(const Identifier& rhs) const
        {
            return filename == rhs.filename && pixel_size == rhs.pixel_size;
        }
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param manager       Font manager.
    /// @param filename      File from which to load the font.
    /// @param pixel_size    Font base size in pixels.
    Font(FontManager& manager, const std::string& filename, const pixel_size_t pixel_size);

public:
    /// Loads a new Font or returns a pointer to an existing font if a font with the same filename /
    /// pixel_size-pair has already been loaded.
    /// @param font_manager     Manager of the loaded Font.
    /// @param filename         File from which the Font is loaded.
    /// @param pixel_size       Nominal size of the loaded Font in pixels.
    static FontPtr load(FontManagerPtr& font_manager, std::string filename, const pixel_size_t pixel_size);

    /// Returns true if this Font is valid.
    /// If the file used to initialize the Font could not be loaded, the Font is invalid.
    bool is_valid() const { return m_face; }

    /// Returns the requested Glyph, or an invalid Glyph if the given codepoint is not known.
    const Glyph& glyph(const codepoint_t codepoint) const;

    /// Font base size in pixels.
    pixel_size_t pixel_size() const { return m_identifier.pixel_size; }

    /// The vertical distance from the horizontal baseline to the hightest ‘character’ coordinate in a font face.
    pixel_size_t ascender() { return m_ascender; }

    /// The vertical distance from the horizontal baseline to the lowest ‘character’ coordinate in a font face.
    pixel_size_t descender() { return m_descender; }

    /// Default vertical baseline-to-baseline distance.
    pixel_size_t line_height() { return m_line_height; }

private:
    /// Renders and returns a new Glyph.
    const Glyph& _allocate_glyph(const codepoint_t codepoint) const;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Font Manager.
    FontManager& m_manager;

    /// Name of the Font.
    std::string m_name;

    /// Every Font is uniquely identified by its file and the Font size in pixels.
    const Identifier m_identifier;

    /// Freetype font face.
    FT_Face m_face;

    /// The vertical distance from the horizontal baseline to the hightest ‘character’ coordinate in a font face.
    pixel_size_t m_ascender;

    /// The vertical distance from the horizontal baseline to the lowest ‘character’ coordinate in a font face.
    pixel_size_t m_descender;

    /// Default vertical baseline-to-baseline distance, usually larger than the sum of the ascender and
    /// descender. There is no guarantee that no glyphs extend above or below subsequent baselines when using this
    /// height.
    pixel_size_t m_line_height;

    /// Glyphs indixed by code point.
    mutable std::unordered_map<codepoint_t, Glyph> m_glyphs;
};

NOTF_CLOSE_NAMESPACE

//== std::hash =======================================================================================================//

namespace std {

/// std::hash specialization for Font::Identifier.
template<>
struct hash<notf::Font::Identifier> {
    size_t operator()(const notf::Font::Identifier& id) const { return notf::hash(id.filename, id.pixel_size); }
};

} // namespace std
