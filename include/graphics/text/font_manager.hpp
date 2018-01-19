#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font.hpp"
#include "graphics/text/font_atlas.hpp"

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

namespace notf {

//====================================================================================================================//

/// @brief Object used to load, render and work with Fonts and rendered text.
class FontManager {

    friend class Font;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(FontManager)

    /// @brief Default constructor.
    FontManager(GraphicsContext& graphics_context);

    /// @brief Destructor.
    ~FontManager();

    /// @brief Direct access to the font atlas texture.
    TexturePtr atlas_texture() const;

private: // for Font
    /// @brief The Freetype library used by the Manager.
    FT_Library freetype() const { return m_freetype; }

    /// @brief Font Atlas to store Glyphs of all loaded Fonts.
    FontAtlas& atlas() { return m_atlas; }

private:
    /// @brief Renders the Font Atlas on screen
    void _debug_render_atlas();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Freetype library used to rasterize the glyphs.
    FT_Library m_freetype;

    /// @brief Render Context in which the Texture lives.
    GraphicsContext& m_graphics_context;

    /// @brief Font Atlas to store Glyphs of all loaded Fonts.
    FontAtlas m_atlas;

    /// @brief All managed Fonts, uniquely identified by a filename/size-pair.
    std::unordered_map<Font::Identifier, std::weak_ptr<Font>> m_fonts;
};

} // namespace notf
