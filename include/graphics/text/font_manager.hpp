#pragma once

#include <string>
#include <unordered_map>

#include "graphics/text/font.hpp"
#include "graphics/text/font_atlas.hpp"

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

namespace notf {

class FontAtlas;
class GraphicsContext;

/**********************************************************************************************************************/

/** Object used to load, render and work with Fonts and rendered text. */
class FontManager {

    friend class Font;

public: // methods
    /** Default constructor. */
    FontManager(GraphicsContext& graphics_context);

    /** Destructor. */
    ~FontManager();

    FontManager(const FontManager&) = delete;            // no copy construction
    FontManager& operator=(const FontManager&) = delete; // no copy assignment

    /** Loads a new Font and returns its FontID.
     * If another Font by the same name is already loaded, it is returned instead an no load is performed.
     */
    FontID load_font(std::string name, std::string filepath, ushort pixel_size);

    /** Returns the FontID of a loaded font, or an invalid FontID if no Font by the name is known. */
    FontID get_font(const std::string& name) const
    {
        const auto& it = m_font_names.find(name);
        if (it != std::end(m_font_names)) {
            return it->second;
        }
        return {};
    } // TODO: FontIDs is broken since you can never remove a font. Is that the idea?

    /** Returns a Font by its ID. */
    const Font& get_font(const FontID id) const { return m_fonts[static_cast<FontID::underlying_t>(id) - 1]; }
    // TODO: unhandled exception if the font ID is not found... maybe Font should behave like Shaders and textures?

    void render_atlas();

    std::shared_ptr<Texture2> get_atlas_texture() const;

private: // for Font
    /** The Freetype library used by the Manager. */
    FT_Library get_freetype() const
    {
        return m_freetype;
    }

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

    /** All managed Fonts. */
    std::vector<Font> m_fonts;

    /** Map of Font names to indices. */
    std::unordered_map<std::string, FontID> m_font_names;
};

} // namespace notf
