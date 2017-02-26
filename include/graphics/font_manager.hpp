#pragma once

struct FT_LibraryRec_;
typedef struct FT_LibraryRec_* FT_Library;

#include "graphics/font_atlas.hpp"
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

    FontManager(const FontManager&) = delete;            // no copy construction
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

/**
 * A Glyph contains information about how to render a single character from a font atlas.
 */
struct Glyph {

    /** Rectangle of the FontAtlas that contains the texture of this Glyph. */
    FontAtlas::Rect rect;

    /** Distance to the left side of the Glyph from the origin in pixels. */
    FontAtlas::coord_t left;

    /** Distance to the top of the Glyph from the baseline in pixels. */
    FontAtlas::coord_t top;

    /** How far to advance the origin horizontal. */
    FontAtlas::coord_t advance_x;

    /** How far to advance the origin vertically. */
    FontAtlas::coord_t advance_y;
};

} // namespace notf
