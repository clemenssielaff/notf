#include "graphics/text/font_manager.hpp"

#include "common/log.hpp"
#include "graphics/text/freetype.hpp"
#include "utils/make_smart_enabler.hpp"

namespace notf {

FontManager::FontManager(GraphicsContext& context)
    : m_freetype(nullptr), m_graphics_context(context), m_atlas(context), m_fonts()
{
    if (FT_Init_FreeType(&m_freetype)) {
        log_critical << "Failed to initialize the Freetype library";
        return;
    }
}

FontManagerPtr FontManager::create(GraphicsContext& context)
{
#ifdef NOTF_DEBUG
    return FontManagerPtr(new FontManager(context));
#else
    return std::make_unique<make_shared_enabler<FontManager>>(context);
#endif
}

FontManager::~FontManager() { FT_Done_FreeType(m_freetype); }

TexturePtr FontManager::atlas_texture() const { return m_atlas.get_texture(); }

} // namespace notf
