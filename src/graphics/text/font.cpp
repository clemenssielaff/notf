#include "graphics/text/font.hpp"

#include <cassert>

#include "common/log.hpp"
#include "common/string.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/text/freetype.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

const Glyph INVALID_GLYPH = {{0, 0, 0, 0}, 0, 0, 0, 0};

} // namespace

NOTF_OPEN_NAMESPACE

static_assert(std::is_pod<Glyph::Rect>::value, "This compiler does not recognize notf::Glyph::Rect as a POD.");

Font::Font(FontManager& manager, const std::string& filename, const pixel_size_t pixel_size)
    : m_manager(manager), m_name(), m_identifier({filename, pixel_size}), m_face(nullptr), m_glyphs()
{
    { // extract the name of the font from the file name
        const std::string basestring = basename(filename.c_str());
        m_name = basestring.substr(0, basestring.rfind('.'));
    }

    // load the new Font into freetype
    FT_Library freetype = m_manager.freetype();
    if (FT_New_Face(freetype, filename.c_str(), 0, &m_face)) {
        log_critical << "Could not load Font from: \"" << filename << "\"";
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, pixel_size);

    { // store font metrics
        assert(m_face->ascender >= 0);
        assert(m_face->height >= 0);
        m_ascender = static_cast<pixel_size_t>(std::abs(m_face->size->metrics.ascender) / 64);
        m_descender = static_cast<pixel_size_t>(std::abs(m_face->size->metrics.descender) / 64);
        m_line_height = static_cast<pixel_size_t>(std::abs(m_face->size->metrics.height) / 64);
    }

    // create the map of glyphs
    FT_GlyphSlot slot = m_face->glyph;
    std::vector<FontAtlas::FitRequest> fit_atlas_request;
    for (codepoint_t codepoint = 32; codepoint < 128; codepoint++) {
        if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER)) {
            log_warning << "Failed to render codepoint " << codepoint << " of Font \"" << m_name << "\"";
            continue;
        }

        Glyph new_glyph;
        new_glyph.rect = {0, 0, 0, 0}; // is determined in the next step
        new_glyph.left = static_cast<Glyph::coord_t>(slot->bitmap_left);
        new_glyph.top = static_cast<Glyph::coord_t>(slot->bitmap_top);
        new_glyph.advance_x = static_cast<Glyph::coord_t>(slot->advance.x / 64);
        new_glyph.advance_y = static_cast<Glyph::coord_t>(slot->advance.y / 64);
        m_glyphs.insert(std::make_pair(codepoint, std::move(new_glyph)));

        fit_atlas_request.emplace_back(FontAtlas::FitRequest{codepoint, static_cast<Glyph::coord_t>(slot->bitmap.width),
                                                             static_cast<Glyph::coord_t>(slot->bitmap.rows)});
    }

    // render the glyph into the atlas
    FontAtlas& font_atlas = m_manager.atlas();
    for (const FontAtlas::ProtoGlyph& protoglyph : font_atlas.insert_rects(std::move(fit_atlas_request))) {
        if (FT_Load_Char(m_face, protoglyph.first, FT_LOAD_RENDER)) {
            continue;
        }
        font_atlas.fill_rect(protoglyph.second, slot->bitmap.buffer);

        assert(m_glyphs.count(protoglyph.first));
        m_glyphs[protoglyph.first].rect = protoglyph.second;
    }

    log_trace << "Loaded Font \"" << m_name << "\" from file: " << filename;
}

FontPtr Font::load(FontManagerPtr& font_manager, std::string filename, const pixel_size_t pixel_size)
{
    const Font::Identifier identifier = {std::move(filename), pixel_size};

    { // check if the given filename/size - pair is already a known (and loaded) font
        auto it = font_manager->m_fonts.find(identifier);
        if (it != font_manager->m_fonts.end()) {
            if (FontPtr font = it->second.lock()) {
                return font;
            }
            else {
                font_manager->m_fonts.erase(it);
            }
        }
    }

    // create and store the new Font in the manager, so it can be re-used
    FontPtr font = NOTF_MAKE_SHARED_FROM_PRIVATE(Font, *font_manager, filename, pixel_size);
    font_manager->m_fonts.insert({identifier, font});
    return font;
}

const Glyph& Font::glyph(const codepoint_t codepoint) const
{
    const auto& it = m_glyphs.find(codepoint);
    if (it != std::end(m_glyphs)) {
        return it->second;
    }
    else {
        return _allocate_glyph(codepoint);
    }
}

const Glyph& Font::_allocate_glyph(const codepoint_t codepoint) const
{
    FT_GlyphSlot slot = m_face->glyph;
    if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER)) {
        log_warning << "Failed to render codepoint " << codepoint << " of Font \"" << m_name << "\"";
        return INVALID_GLYPH;
    }

    Glyph glyph;
    glyph.left = static_cast<Glyph::coord_t>(slot->bitmap_left);
    glyph.top = static_cast<Glyph::coord_t>(slot->bitmap_top);
    glyph.advance_x = static_cast<Glyph::coord_t>(slot->advance.x >> 6); // "x >> 6" is "x / 64" in fancy
    glyph.advance_y = static_cast<Glyph::coord_t>(slot->advance.y >> 6);

    if (slot->bitmap.width > 0) {
        assert(slot->bitmap.rows);
        FontAtlas& font_atlas = m_manager.atlas();
        glyph.rect = font_atlas.insert_rect(static_cast<Glyph::coord_t>(slot->bitmap.width),
                                            static_cast<Glyph::coord_t>(slot->bitmap.rows));
        font_atlas.fill_rect(glyph.rect, slot->bitmap.buffer);
    }
    else {
        assert(!slot->bitmap.rows);
        glyph.rect.x = 0;
        glyph.rect.y = 0;
        glyph.rect.height = 0;
        glyph.rect.width = 0;
    }

    // store and return the new Glyph
    return m_glyphs.insert(std::make_pair(codepoint, std::move(glyph))).first->second;
}

NOTF_CLOSE_NAMESPACE
