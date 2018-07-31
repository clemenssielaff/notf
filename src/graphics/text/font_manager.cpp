#include "graphics/text/font_manager.hpp"

#include "common/log.hpp"
#include "graphics/text/freetype.hpp"

NOTF_OPEN_NAMESPACE

FontManager::FontManager() : m_freetype(nullptr), m_atlas()
{
    if (FT_Init_FreeType(&m_freetype)) {
        log_critical << "Failed to initialize the Freetype library";
        return;
    }
}

FontManager::~FontManager() { FT_Done_FreeType(m_freetype); }

TexturePtr FontManager::get_atlas_texture() const { return m_atlas.get_texture(); }

NOTF_CLOSE_NAMESPACE
