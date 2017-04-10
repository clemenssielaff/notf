#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font_atlas.hpp"
#include "graphics/text/font_id.hpp"

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace notf {

class FontManager;

/**********************************************************************************************************************/

/**
 * A Glyph contains information about how to render a single character from a font atlas.
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
 *        --X----------|-----0MMMMMMMWNKOdl,.-----|-----|--X--->
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
 *          |- - - - - - - - - - - - - - - - - - - - - - -+
 *          |                 -advance_x->
 *          v
 */

struct Glyph {
    using pos_t = int;

    /** Rectangle of the FontAtlas that contains the texture of this Glyph. */
    FontAtlas::Rect rect;

    /** Distance to the left side of the Glyph from the origin in pixels. */
    FontAtlas::coord_t left;

    /** Distance to the top of the Glyph from the baseline in pixels. */
    FontAtlas::coord_t top;

    /** How far to advance the origin horizontal. */
    pos_t advance_x;

    /** How far to advance the origin vertically. */
    pos_t advance_y;
};

/**********************************************************************************************************************/

/** A Font is a manger object for a given font face in the RenderManager.
 * It knows where its Glyphs reside in the RenderManager's Font Atlas.
 * At the moment, a Font renders all renderable ascii Glyphs to begin with and adds new Glyphs should they be requested.
 */
class Font {

public: // methods
    /** Constructor. */
    Font(FontManager* manager, const FontID id, const std::string name,
         const std::string filename, const ushort pixel_size);

    /** Returns true if this Font is valid.
     * If the file used to initialize the Font could not be loaded, the Font is invalid.
     */
    bool is_valid() const { return m_face; }

    /** Returns the requested Glyph, or an invalid Glyph if the given codepoint is not known. */
    const Glyph& get_glyph(const codepoint_t codepoint) const;

private: // methods
    /** Renders and returns a new Glyph. */
    const Glyph& _allocate_glyph(const codepoint_t codepoint);

private: // members
    /** Font Manager. */
    FontManager* m_manager;

    /** Id of this Font. */
    const FontID m_id;

    /** Given name of the Font. */
    const std::string m_name;

    /** Filename of the loaded Font. */
    const std::string m_filename;

    /** Pixel size of this Font. */
    const ushort m_pixel_size;

    /** Freetype font face. */
    FT_Face m_face;

    /** Glyphs indixed by code point. */
    mutable std::unordered_map<codepoint_t, Glyph> m_glyphs;
};

} // namespace notf
