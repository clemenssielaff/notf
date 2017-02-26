#pragma once

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

#include "graphics/shader.hpp"

namespace notf {

/** Object used to load, render and work with Fonts and rendered text.
 * Before creating an instance of this class, make sure that a valid OpenGL context exists.
 */
class FontManager {

public: // methods
    /** Default constructor. */
    FontManager();

    /** Destructor. */
    ~FontManager();

    FontManager(const FontManager&) = delete; // no copy construction
    FontManager& operator=(const FontManager&) = delete; // no copy assignment

private: // fields
    /** Freetype library used to rasterize the glyphs. */
    FT_Library m_freetype;

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
