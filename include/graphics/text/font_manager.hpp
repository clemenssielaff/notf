#pragma once

#include <string>
#include <unordered_map>

#include "common/size2i.hpp"
#include "common/vector2.hpp"
#include "graphics/shader.hpp"
#include "graphics/text/font.hpp"
#include "graphics/text/font_atlas.hpp"

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

namespace notf {

/** To allow other objects to keep references to a Font without handing them volatile pointers or split ownership with
 * shared pointers, everyone can request a `FontID` from the manager.
 * The FontId corresponds to an index in a vector in the FontManager, meaning lookup of the actual Font object given its
 * ID is trivial.
 * ID zero is reserved as INVALID_FONT.
 */
using FontID              = size_t;
const FontID INVALID_FONT = 0;

class FontAtlas;

/**********************************************************************************************************************/

/** Object used to load, render and work with Fonts and rendered text.
 * Before creating an instance of this class, make sure that a valid OpenGL context exists.
 */
class FontManager {

    friend Font::Font(FontManager*, std::string, ushort);

public: // methods
    /** Default constructor. */
    FontManager();

    /** Destructor. */
    ~FontManager();

    FontManager(const FontManager&) = delete;            // no copy construction
    FontManager& operator=(const FontManager&) = delete; // no copy assignment

    /** Loads a new Font and returns its FontID.
     * If another Font by the same name is already loaded, it is returned instead an no load is performed.
     */
    FontID load_font(std::string name, std::string filepath);

    /** Returns the FontID of a loaded font, or INVALID_FONT if no Font by the name is known. */
    FontID get_font(const std::string& name) const
    {
        const auto& it = m_font_names.find(name);
        if (it != std::end(m_font_names)) {
            return it->second;
        }
        return INVALID_FONT;
    }

    /** Sets the size of the Window into which to render text.
     * Is necessary to construct the correct projection matrix.
     * Must be called before the first text is rendered into the window.
     */
    void set_window_size(Size2i size) { m_window_size = std::move(size); }

    /** Remders a text at the given screen coordinate.
     * The position corresponts to the start of the text's baseline.
     */
    void render_text(const std::string& text, ushort x, ushort y, const FontID font_id);

private: // for Font
    /** The Freetype library used by the Manager. */
    FT_Library get_freetype() const
    {
        return m_freetype;
    }

    /** Font Atlas to store Glyphs of all loaded Fonts. */
    FontAtlas& get_atlas() { return m_atlas; }

private: // fields
    /** Freetype library used to rasterize the glyphs. */
    FT_Library m_freetype;

    /** Font Atlas to store Glyphs of all loaded Fonts. */
    FontAtlas m_atlas;

    /** All managed Fonts. */
    std::vector<Font> m_fonts;

    /** Map of Font names to indices. */
    std::unordered_map<std::string, FontID> m_font_names;

    /** Current size of the Window into which text is rendered. */
    Size2i m_window_size;

    // Shader and -related //

    /** The Shader program used to render the font. */
    Shader m_font_shader;

    /** Color uniform, is a color value. */
    GLint m_color_uniform;

    /** Color uniform, is an integer value. */
    GLint m_texture_id_uniform;

    /** View-projection matrix uniform of the 'camera' seeing the text, is a matrix4f value. */
    GLint m_view_proj_matrix_uniform;

    /** World matrix uniform of the rendered text, is a matrix4f value. */
    GLint m_world_matrix_uniform;
};

} // namespace notf
