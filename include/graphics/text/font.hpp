#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/hash.hpp"
#include "common/utf.hpp"

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace notf {

class FontManager;
class GraphicsContext;

class Font;
using FontPtr = std::shared_ptr<Font>;

/** Data type to identify a single Glyph. */
using codepoint_t = utf32_t;

/**********************************************************************************************************************/

/**
 * A Glyph contains information about how to render a single character from a font atlas.
 * Glyph coordinates are stored as signed integers, because they can be negative as well.
 *
 *          ^
 *          |
 *          |                <-rect.width->
 *          |          +--------------------------+
 *          |          |        :xKWMMWKd,        |
 *          |          |    c0MMMKxodOWMMW0OOOOOOO|
 *          |          |  ;WMMMO.      dMMMMWdoooo|     ^
 *          |     ^    | ;MMMM0         xMMMMo    |     |
 *          |     |    | 0MMMMl         'MMMMW    |     |
 *          |    top   | OMMMMd         :MMMMN    |     |
 *          |     |    | .WMMMW.       .XMMMW;    |     |
 *          |          |  .OMMMWd'   .lNMMMk.     |  rect.height
 *          |          |    .ckXMMMMMMN0x:        |     |
 *          |          |      'XMK..              |     |
 *          |  -left-> |     :MMMk.               |     |
 *        --X----------|-----0MMMMMMMWNKOdl,.-----|-----|--x--->
 *   origin |          |    'kMMNKXWMMMMMMMMMXo   |     |  |
 *          |          | ,0MMWo.      .,cxNMMMMN. |     v
 *          |          |oMMMM.             oMMMMd |        |
 *          |          |WMMMW              .MMMM' |
 *          |          |XMMMMd            .KMMW;  |        |
 *          |          |.XMMMMK:.       'dWMNo    |
 *          |          |  ;OWMMMMN0OOOXMW0o'      |        |
 *          |          |     ;xKWMMMN0d,          |
 *          |          +--------------------------+        |
 *          |
 *          |- - - - - - - - - - - - - - - - - - - - - - - +
 *          |                 -advance_x->
 *          v
 */
struct Glyph {

public: // types ******************************************************************************************************/
    /** Integer type to store a single Glyph coordinate. */
    using coord_t = int16_t;

    /** Integer type that can be used to express an area (coordinate_type^2). */
    using area_t = int32_t;

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

public: // fields *****************************************************************************************************/
    /** Rectangle of the FontAtlas that contains the texture of this Glyph. */
    Rect rect;

    /** Distance to the left side of the Glyph from the origin in pixels. */
    coord_t left;

    /** Distance to the top of the Glyph from the baseline in pixels. */
    coord_t top;

    /** How far to advance the origin horizontal. */
    coord_t advance_x;

    /** How far to advance the origin vertically. */
    coord_t advance_y;
};

/**********************************************************************************************************************/

/** A Font is a manger object for a given font face in the RenderManager.
 * It knows where its Glyphs reside in the RenderManager's Font Atlas.
 * At the moment, a Font renders all renderable ascii Glyphs to begin with and adds new Glyphs should they be requested.
 */
class Font {

    friend class FontManager;

public: // types
    using pixel_size_t = ushort;

    /** Every Font is uniquely identified by its file and the Font size in pixels. */
    struct Identifier {

        /** Filename of the loaded Font. */
        std::string filename;

        /** Pixel size of this Font. */
        pixel_size_t pixel_size;

        /** Equality test operator. f*/
        bool operator==(const Identifier& rhs) const
        {
            return filename == rhs.filename && pixel_size == rhs.pixel_size;
        }
    };

public: // static methods *********************************************************************************************/
    /** Loads a new Font or returns a pointer to an existing font if a font with the same filename / pixel_size-pair
     * has already been loaded.
     * @param context       Render Context in which the Font lives.
     * @param filename      File from which the Font is loaded.
     * @param pixel_size    Nominal size of the loaded Font in pixels.
     */
    static std::shared_ptr<Font> load(
        GraphicsContext& context, const std::string filename, const pixel_size_t pixel_size);

private: // constructor ***********************************************************************************************/
    /** Constructor.
     * @param manager       Font manager.
     * @param filename      File from which to load the font.
     * @param pixel_size    Font base size in pixels.
     */
    Font(FontManager& manager, const std::string filename, const pixel_size_t pixel_size);

public: // methods ****************************************************************************************************/
    /** Returns true if this Font is valid.
     * If the file used to initialize the Font could not be loaded, the Font is invalid.
     */
    bool is_valid() const { return m_face; }

    /** Returns the requested Glyph, or an invalid Glyph if the given codepoint is not known. */
    const Glyph& get_glyph(const codepoint_t codepoint) const;

private: // methods ***************************************************************************************************/
    /** Renders and returns a new Glyph. */
    const Glyph& _allocate_glyph(const codepoint_t codepoint) const;

private: // members ***************************************************************************************************/
    /** Font Manager. */
    FontManager& m_manager;

    /** Name of the Font. */
    std::string m_name;

    /** Every Font is uniquely identified by its file and the Font size in pixels. */
    const Identifier m_identifier;

    /** Freetype font face. */
    FT_Face m_face;

    /** Glyphs indixed by code point. */
    mutable std::unordered_map<codepoint_t, Glyph> m_glyphs;
};

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Font::Identifier. */
template <>
struct hash<notf::Font::Identifier> {
    size_t operator()(const notf::Font::Identifier& id) const { return notf::hash(id.filename, id.pixel_size); }
};

} // namespace std
