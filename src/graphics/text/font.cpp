#include "graphics/text/font.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/text/freetype.hpp"

namespace { // anonymous
using namespace notf;

const Glyph INVALID_GLYPH = {{0, 0, 0, 0}, 0, 0, 0, 0};

} // namespace anonymous

namespace notf {

Font::Font(FontManager* manager, const FontID id, const std::string name,
           const std::string filename, const ushort pixel_size)
    : m_manager(manager)
    , m_id(id)
    , m_name(std::move(name))
    , m_filename(std::move(filename))
    , m_pixel_size(pixel_size)
    , m_face(nullptr)
    , m_glyphs()
{
    FT_Library freetype   = m_manager->get_freetype();
    FontAtlas& font_atlas = m_manager->get_atlas();

    // load the font into freetype
    if (FT_New_Face(freetype, m_filename.c_str(), 0, &m_face)) {
        log_critical << "Could not load Font from: " << m_filename;
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, m_pixel_size);

    // create the map of glyphs
    FT_GlyphSlot slot = m_face->glyph;
    std::vector<FontAtlas::FitRequest> fit_atlas_request;
    for (codepoint_t codepoint = 32; codepoint < 128; codepoint++) {
        if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER)) {
            log_warning << "Failed to render codepoint " << codepoint << " of Font \"" << m_name << "\"";
            continue;
        }

        Glyph new_glyph;
        new_glyph.rect      = {0, 0, 0, 0}; // is determined in the next step
        new_glyph.left      = static_cast<FontAtlas::coord_t>(slot->bitmap_left);
        new_glyph.top       = static_cast<FontAtlas::coord_t>(slot->bitmap_top);
        new_glyph.advance_x = static_cast<Glyph::pos_t>(slot->advance.x >> 6);
        new_glyph.advance_y = static_cast<Glyph::pos_t>(slot->advance.y >> 6);
        m_glyphs.insert(std::make_pair(codepoint, std::move(new_glyph)));

        fit_atlas_request.emplace_back(FontAtlas::FitRequest{codepoint,
                                                             static_cast<FontAtlas::coord_t>(slot->bitmap.width),
                                                             static_cast<FontAtlas::coord_t>(slot->bitmap.rows)});
    }

    // render the glyph into the atlas
    for (const FontAtlas::ProtoGlyph& protoglyph : font_atlas.insert_rects(std::move(fit_atlas_request))) {
        if (FT_Load_Char(m_face, protoglyph.first, FT_LOAD_RENDER)) {
            continue;
        }
        font_atlas.fill_rect(protoglyph.second, slot->bitmap.buffer);

        assert(m_glyphs.count(protoglyph.first));
        m_glyphs[protoglyph.first].rect = protoglyph.second;
    }
}

const Glyph& Font::get_glyph(const codepoint_t codepoint) const
{
    const auto& it = m_glyphs.find(codepoint);
    if (it != std::end(m_glyphs)) {
        return it->second;
    }
    log_warning << "Could not find Glyph " << codepoint << " of Font \"" << m_name << "\""; // TODO: dynamically allocate glyphs (almost done, test)
    return INVALID_GLYPH;
}

const Glyph& Font::_allocate_glyph(const codepoint_t codepoint)
{
    FT_GlyphSlot slot = m_face->glyph;
    if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER)) {
        log_warning << "Failed to render codepoint " << codepoint << " of Font \"" << m_name << "\"";
        return INVALID_GLYPH;
    }

    Glyph glyph;
    glyph.left      = static_cast<FontAtlas::coord_t>(slot->bitmap_left);
    glyph.top       = static_cast<FontAtlas::coord_t>(slot->bitmap_top);
    glyph.advance_x = static_cast<Glyph::pos_t>(slot->advance.x >> 6);
    glyph.advance_y = static_cast<Glyph::pos_t>(slot->advance.y >> 6);

    FontAtlas& font_atlas = m_manager->get_atlas();
    glyph.rect            = font_atlas.insert_rect(static_cast<FontAtlas::coord_t>(slot->bitmap.width),
                                        static_cast<FontAtlas::coord_t>(slot->bitmap.rows));
    font_atlas.fill_rect(glyph.rect, slot->bitmap.buffer);

    m_glyphs.insert(std::make_pair(codepoint, std::move(glyph)));

    return m_glyphs.at(codepoint);
}

} // namespace notf
