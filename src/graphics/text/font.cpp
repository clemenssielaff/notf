#include "graphics/text/font.hpp"

#include "common/log.hpp"
#include "common/string_utils.hpp"
#include "common/system.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/text/freetype.hpp"

namespace notf {

Font::Font(FontManager* manager, std::string filename, ushort pixel_size)
    : m_filename(std::move(filename))
    , m_pixel_size(pixel_size)
    , m_face(nullptr)
    , m_glyphs()
{
    FT_Library freetype = manager->get_freetype();

    // load the font into freetype
    if (FT_New_Face(freetype, m_filename.c_str(), 0, &m_face)) {
        log_critical << "Could not load Font from: " << m_filename;
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, m_pixel_size);

    // construct the rectangles to fit into the font atlas
    std::vector<FontAtlas::NamedExtend> request;
    FT_GlyphSlot slot = m_face->glyph;
    for (codepoint_t codepoint = 32; codepoint < 128; codepoint++) {
        if (FT_Load_Char(m_face, codepoint, FT_LOAD_DEFAULT)) {
            std::string msg = string_format("Failed to load character %c of font %s", codepoint, m_filename.c_str());
            log_warning << msg;
        }
        request.emplace_back(codepoint, slot->bitmap.width, slot->bitmap.rows);
    }

    // fit the rectangles
    FontAtlas& font_atlas                         = manager->get_atlas();
    std::vector<FontAtlas::NamedRect> glyph_atlas = font_atlas.insert_rects(std::move(request));

    // create the map of glyphs
    for (const FontAtlas::NamedRect& protoglyph : glyph_atlas) {

        if (FT_Load_Char(m_face, protoglyph.code_point, FT_LOAD_RENDER)) {
            continue;
        }
        Glyph new_glyph;
        new_glyph.rect      = protoglyph.rect;
        new_glyph.left      = static_cast<FontAtlas::coord_t>(slot->bitmap_left);
        new_glyph.top       = static_cast<FontAtlas::coord_t>(slot->bitmap_top);
        new_glyph.advance_x = static_cast<fontpos_t>(slot->advance.x >> 6);
        new_glyph.advance_y = static_cast<fontpos_t>(slot->advance.y >> 6);
        m_glyphs.insert(std::make_pair(protoglyph.code_point, std::move(new_glyph)));

        // render the glyph into the atlas
        font_atlas.fill_rect(protoglyph.rect, slot->bitmap.buffer);
    }
}

} // namespace notf
