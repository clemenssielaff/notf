#include "graphics/text/font_manager.hpp"

#include "common/log.hpp"
#include "graphics/text/freetype.hpp"

namespace notf {

FontManager::FontManager(GraphicsContext& graphics_context)
    : m_freetype(nullptr), m_graphics_context(graphics_context), m_atlas(graphics_context), m_fonts()
{
    if (FT_Init_FreeType(&m_freetype)) {
        log_critical << "Failed to initialize the Freetype library";
        return;
    }
}

FontManager::~FontManager() { FT_Done_FreeType(m_freetype); }

TexturePtr FontManager::atlas_texture() const { return m_atlas.get_texture(); }

} // namespace notf