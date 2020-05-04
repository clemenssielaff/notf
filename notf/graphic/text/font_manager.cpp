#include "notf/graphic/text/font_manager.hpp"

#include "notf/graphic/text/freetype.hpp"
#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

FontManager::FontManager() : m_freetype(nullptr), m_atlas() {
    if (FT_Init_FreeType(&m_freetype)) {
        NOTF_LOG_CRIT("Failed to initialize the Freetype library");
        return;
    }
}

FontManager::~FontManager() { FT_Done_FreeType(m_freetype); }

TexturePtr FontManager::get_atlas_texture() const { return m_atlas.get_texture(); }

NOTF_CLOSE_NAMESPACE
