#include "graphics/text/font.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/string.hpp"
#include "common/system.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/text/freetype.hpp"

namespace notf {

Font::Font(FontManager* manager, FontID id, std::__cxx11::string name, std::string filename, ushort pixel_size)
    : m_id(id)
    , m_name(std::move(name))
    , m_filename(std::move(filename))
    , m_pixel_size(pixel_size)
    , m_face(nullptr)
    , m_glyphs()
{
    FT_Library freetype   = manager->get_freetype();
    FontAtlas& font_atlas = manager->get_atlas();

    // load the font into freetype
    if (FT_New_Face(freetype, m_filename.c_str(), 0, &m_face)) {
        log_critical << "Could not load Font from: " << m_filename;
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, m_pixel_size);

    // create the map of glyphs
    FT_GlyphSlot slot = m_face->glyph;
    std::vector<FontAtlas::NamedExtend> fit_atlas_request;
    for (codepoint_t code_point = 32; code_point < 128; code_point++) {
        if (FT_Load_Char(m_face, code_point, FT_LOAD_RENDER)) {
            log_warning << "Failed to render codepoint " << code_point << " of Font \"" << m_name << "\"";
            continue;
        }

        Glyph new_glyph;
        new_glyph.rect      = {0, 0, 0, 0};
        new_glyph.left      = static_cast<FontAtlas::coord_t>(slot->bitmap_left);
        new_glyph.top       = static_cast<FontAtlas::coord_t>(slot->bitmap_top);
        new_glyph.advance_x = static_cast<fontpos_t>(slot->advance.x >> 6);
        new_glyph.advance_y = static_cast<fontpos_t>(slot->advance.y >> 6);
        m_glyphs.insert(std::make_pair(code_point, std::move(new_glyph)));

        fit_atlas_request.emplace_back(code_point, slot->bitmap.width, slot->bitmap.rows);
    }

    // render the glyph into the atlas
    for (const FontAtlas::NamedRect& protoglyph : font_atlas.insert_rects(std::move(fit_atlas_request))) {
        if (FT_Load_Char(m_face, protoglyph.code_point, FT_LOAD_RENDER)) {
            continue;
        }
        font_atlas.fill_rect(protoglyph.rect, slot->bitmap.buffer);

        assert(m_glyphs.count(protoglyph.code_point));
        m_glyphs.at(protoglyph.code_point).rect = protoglyph.rect;
    }
}

const Glyph& Font::get_glyph(const codepoint_t codepoint) const
{
    static Glyph error_glyph = {{0, 0, 0, 0}, 0, 0, 0, 0};

    const auto& it = m_glyphs.find(codepoint);
    if (it != std::end(m_glyphs)) {
        return it->second;
    }
    log_warning << "Could not find Glyph " << codepoint << " of Font \"" << m_name << "\"";
    return error_glyph;
}

} // namespace notf
