#include "graphics/text/font.hpp"

#include <assert.h>

#include "common/log.hpp"
#include "common/string.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/text/font_manager.hpp"
#include "graphics/text/freetype.hpp"

namespace { // anonymous
using namespace notf;

const Glyph INVALID_GLYPH = {{0, 0, 0, 0}, 0, 0, 0, 0};

} // namespace anonymous

namespace notf {

static_assert(std::is_pod<Glyph::Rect>::value, "This compiler does not recognize notf::Glyph::Rect as a POD.");

std::shared_ptr<Font> Font::load(GraphicsContext& context, const std::string filename, const pixel_size_t pixel_size)
{
    FontManager& font_manager         = context.get_font_manager();
    const Font::Identifier identifier = {filename, pixel_size};

    { // check if the given filename/size - pair is already a known (and loaded) font
        auto it = font_manager.m_fonts.find(identifier);
        if (it != font_manager.m_fonts.end()) {
            if (std::shared_ptr<Font> font = it->second.lock()) {
                return font;
            }
            else {
                font_manager.m_fonts.erase(it);
            }
        }
    }

    // create and store the new Font in the manager, so it can be re-used
    struct make_shared_enabler : public Font {
        make_shared_enabler(FontManager& manager, const std::string filename, const pixel_size_t pixel_size)
            : Font(manager, filename, pixel_size) {}
    };
    std::shared_ptr<Font> font = std::make_shared<make_shared_enabler>(font_manager, filename, pixel_size);
    font_manager.m_fonts.insert({identifier, font});
    return font;
}

Font::Font(FontManager& manager, const std::string filename, const pixel_size_t pixel_size)
    : m_manager(manager)
    , m_name()
    , m_identifier({filename, pixel_size})
    , m_face(nullptr)
    , m_glyphs()
{
    { // extract the name of the font from the file name
        const std::string basestring = basename(filename.c_str());
        m_name                       = basestring.substr(0, basestring.rfind('.'));
    }

    // load the new Font into freetype
    FT_Library freetype = m_manager.get_freetype();
    if (FT_New_Face(freetype, filename.c_str(), 0, &m_face)) {
        log_critical << "Could not load Font from: " << filename;
        return;
    }
    FT_Set_Pixel_Sizes(m_face, 0, pixel_size);

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
        new_glyph.left      = static_cast<Glyph::coord_t>(slot->bitmap_left);
        new_glyph.top       = static_cast<Glyph::coord_t>(slot->bitmap_top);
        new_glyph.advance_x = static_cast<Glyph::coord_t>(slot->advance.x >> 6);
        new_glyph.advance_y = static_cast<Glyph::coord_t>(slot->advance.y >> 6);
        m_glyphs.insert(std::make_pair(codepoint, std::move(new_glyph)));

        fit_atlas_request.emplace_back(FontAtlas::FitRequest{
            codepoint,
            static_cast<Glyph::coord_t>(slot->bitmap.width),
            static_cast<Glyph::coord_t>(slot->bitmap.rows)});
    }

    // render the glyph into the atlas
    FontAtlas& font_atlas = m_manager.get_atlas();
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

const Glyph& Font::get_glyph(const codepoint_t codepoint) const
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
    glyph.left      = static_cast<Glyph::coord_t>(slot->bitmap_left);
    glyph.top       = static_cast<Glyph::coord_t>(slot->bitmap_top);
    glyph.advance_x = static_cast<Glyph::coord_t>(slot->advance.x >> 6); // "x >> 6" is "x / 64" in fancy
    glyph.advance_y = static_cast<Glyph::coord_t>(slot->advance.y >> 6);

    if (slot->bitmap.width > 0) {
        assert(slot->bitmap.rows);
        FontAtlas& font_atlas = m_manager.get_atlas();
        glyph.rect            = font_atlas.insert_rect(
            static_cast<Glyph::coord_t>(slot->bitmap.width),
            static_cast<Glyph::coord_t>(slot->bitmap.rows));
        font_atlas.fill_rect(glyph.rect, slot->bitmap.buffer); // TODO: dynamically allocating glyphs fails with OpenGL error: GL_INVALID_OPERATION
    }
    else {
        assert(!slot->bitmap.rows);
    }

    // store and return the new Glyph
    return m_glyphs.insert(std::make_pair(codepoint, std::move(glyph))).first->second;
}

} // namespace notf
