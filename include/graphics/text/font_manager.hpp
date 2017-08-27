#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font.hpp"
#include "graphics/text/font_atlas.hpp"

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

namespace notf {

class Font;
class FontAtlas;
class GraphicsContext;

/**********************************************************************************************************************/

/** Object used to load, render and work with Fonts and rendered text. */
class FontManager {

    friend class Font;

public: // methods
    DISALLOW_COPY_AND_ASSIGN(FontManager)

    /** Default constructor. */
    FontManager(GraphicsContext& graphics_context);

    /** Destructor. */
    ~FontManager();

    /** Direct access to the font atlas texture. */
    std::shared_ptr<Texture2> get_atlas_texture() const;

private: // for Font
    /** The Freetype library used by the Manager. */
    FT_Library get_freetype() const { return m_freetype; }

    /** Font Atlas to store Glyphs of all loaded Fonts. */
    FontAtlas& get_atlas() { return m_atlas; }

private: // methods
    /** Renders the Font Atlas on screen */
    void _debug_render_atlas();

private: // fields
    /** Freetype library used to rasterize the glyphs. */
    FT_Library m_freetype;

    /** Render Context in which the Texture lives. */
    GraphicsContext& m_graphics_context;

    /** Font Atlas to store Glyphs of all loaded Fonts. */
    FontAtlas m_atlas;

    /** All managed Fonts, uniquely identified by a filename/size-pair. */
    std::unordered_map<Font::Identifier, std::weak_ptr<Font>> m_fonts;
};

} // namespace notf
