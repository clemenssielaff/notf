#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font_atlas.hpp"

struct FT_FaceRec_;
typedef struct FT_FaceRec_* FT_Face;

namespace notf {

class FontManager;

using fontpos_t = int;

/**********************************************************************************************************************/

/**
 * A Glyph contains information about how to render a single character from a font atlas.
 */
struct Glyph {

    /** Rectangle of the FontAtlas that contains the texture of this Glyph. */
    FontAtlas::Rect rect;

    /** Distance to the left side of the Glyph from the origin in pixels. */
    FontAtlas::coord_t left;

    /** Distance to the top of the Glyph from the baseline in pixels. */
    FontAtlas::coord_t top;

    /** How far to advance the origin horizontal. */
    fontpos_t advance_x;

    /** How far to advance the origin vertically. */
    fontpos_t advance_y;
};

/**********************************************************************************************************************/

class Font {

public: // methods
    /** Constructor. */
    Font(FontManager* manager, std::string filename, ushort pixel_size);

    /** Returns true if this Font is valid. */
    bool is_valid() const { return m_face; }

    /** Returns the FontID of a loaded font, or INVALID_FONT if no Font by the name is known. */
    const Glyph& get_glyph(const codepoint_t codepoint) const
    {
        static Glyph error_glyph = {{0, 0, 0, 0}, 0, 0, 0, 0};

        const auto& it = m_glyphs.find(codepoint);
        if (it != std::end(m_glyphs)) {
            return it->second;
        }
        return error_glyph;
    }

private: // members
    /** Filename of the loaded Font. */
    const std::string m_filename;

    /** Pixel size of this Font. */
    const ushort m_pixel_size;

    /** Freetype font face. */
    FT_Face m_face;

    /** Glyphs indixed by code point. */
    std::unordered_map<codepoint_t, Glyph> m_glyphs;
};

} // namespace notf
