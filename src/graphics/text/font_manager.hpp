#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font_atlas.hpp"

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Object used to load, render and work with Fonts and rendered text.
class FontManager {

    friend class Font;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE;

    /// Constructor.
    FontManager();

public:
    NOTF_NO_COPY_OR_ASSIGN(FontManager);

    /// Factory
    static FontManagerPtr create() { return NOTF_MAKE_UNIQUE_FROM_PRIVATE(FontManager); }

    /// Destructor.
    ~FontManager();

    /// Direct access to the font atlas texture.
    TexturePtr get_atlas_texture() const;

private: // for Font
    /// The Freetype library used by the Manager.
    FT_Library freetype() const { return m_freetype; }

    /// Font Atlas to store Glyphs of all loaded Fonts.
    FontAtlas& atlas() { return m_atlas; }

private:
    /// Renders the Font Atlas on screen.
    void _debug_render_atlas();

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Freetype library used to rasterize the glyphs.
    FT_Library m_freetype;

    /// Font Atlas to store Glyphs of all loaded Fonts.
    FontAtlas m_atlas;

    /// All managed Fonts, uniquely identified by a filename/size-pair.
    std::unordered_map<Font::Identifier, FontWeakPtr> m_fonts;
};

NOTF_CLOSE_NAMESPACE
