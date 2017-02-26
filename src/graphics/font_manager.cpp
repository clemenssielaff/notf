#include "graphics/font_manager.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "common/log.hpp"
#include "common/system.hpp"
#include "core/glfw_wrapper.hpp"

namespace { // anonymous
// clang-format off

const char* font_vertex_shader =
#include "shader/font.vert"

// fragment shader source
const char* font_fragment_shader =
#include "shader/font.frag"

// clang-format on
} // namespace anonymous

namespace notf {

FontManager::FontManager()
    : m_freetype(nullptr)
    , m_font_shader(Shader::build("font_shader", font_vertex_shader, font_fragment_shader))
    , m_color_uniform(0)
    , m_texture_id_uniform(0)
    , m_view_proj_matrix_uniform(0)
    , m_world_matrix_uniform(0)
{
    if (FT_Init_FreeType(&m_freetype)) {
        log_critical << "Failed to initialize the Freetype library";
        return;
    }

    if (!m_font_shader.is_valid()) {
        log_critical << "Failed to create Font shader";
        return;
    }
    m_color_uniform            = glGetUniformLocation(m_font_shader.get_id(), "color");
    m_texture_id_uniform       = glGetUniformLocation(m_font_shader.get_id(), "tex");
    m_view_proj_matrix_uniform = glGetUniformLocation(m_font_shader.get_id(), "view_proj_matrix");
    m_world_matrix_uniform     = glGetUniformLocation(m_font_shader.get_id(), "world_matrix");
}

FontManager::~FontManager()
{
    FT_Done_FreeType(m_freetype);
}

} // namespace notf
