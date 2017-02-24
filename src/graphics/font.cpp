#if 0
#include "graphics/font.hpp"

//#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb_truetype/stb_rect_pack.h"
#include "thirdparty/stb_truetype/stb_truetype.h"

#include "common/log.hpp"
#include "common/system.hpp"

namespace notf {

Font Font::load(FontInfo info)
{
    Font font(std::move(info));

    // load the file
    const std::string file_contents = load_file(font.get_filepath());
    if (file_contents.empty()) {
        log_critical << "Cannot create font: \"" << font.get_name() << "\" without data";
        return font;
    }
    if (!stbtt_InitFont(&font.m_font, reinterpret_cast<const unsigned char*>(file_contents.c_str()), 0)) {
        log_critical << "Failed to initialize Font context for font: \"" << font.get_name() << "\"";
        return font;
    }

    // create the atlas
    const Size2i& atlas_size = font.get_atlas_size();
    if(!atlas_size.is_valid()){
        log_critical << "Cannot create Font : \"" << font.get_name() << "\" with an invalid atlas size: " << atlas_size;
        return font;
    }
    std::vector<unsigned char> atlas_pixels(atlas_size.get_area());
    if (!stbtt_PackBegin(&font.m_pack, &atlas_pixels[0], atlas_size.width, atlas_size.height, 0, 1, nullptr)) {
        log_critical << "Failed to create atlas for Font: \"" << font.get_name() << "\"";
        return font;
    }
    stbtt_PackSetOversampling(&font.m_pack, /* horizonal = */ 2, /* vertical = */ 2);

    { // calculate the normalized font metrics
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font.m_font, &ascent, &descent, &line_gap);
        float font_height = static_cast<float>(ascent - descent);
        if (font_height > 0) {
            font.m_ascender  = static_cast<float>(ascent) / font_height;
            font.m_descender = static_cast<float>(descent) / font_height;

            // the real line height is calculated by multiplying the line height by font size
            font.m_line_height = (font_height + static_cast<float>(line_gap)) / font_height;
        }
    }

    return font;
}

Font::Font(FontInfo info)
    : m_font{nullptr, nullptr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    , m_pack{nullptr, nullptr, 0, 0, 0, 0, 0, 0, nullptr, nullptr}
    , m_info(std::move(info))
    , m_glyphs()
    , m_ascender(0)
    , m_descender(0)
    , m_line_height(0)
{
}

Font::~Font()
{
    stbtt_PackEnd(&m_pack);
}

} // namespace notf

/*
 * It seems like NanoVG is using a single atlas for ALL fonts, which makes total sense.
 * Also, it adds the ability to blur glyphs, something that I need as well (for shadows and just in general)
 * NanoVG adds the blurred glyphs to the atlas as well, which I cannot do using the integrated packer, as the blurring
 * happens after the rendering and would require me to manually insert the new glyph .. which might be possible.
 * Or I can supply my own packer like NanoVG does.
 * Or I achieve blur using a shader effect (which would also allow stuff like "pulsing" or other animated blurs).
 */

#endif
