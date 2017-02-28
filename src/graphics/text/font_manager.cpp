#include "graphics/text/font_manager.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/system.hpp"
#include "common/transform3.hpp"
#include "core/glfw_wrapper.hpp"
#include "graphics/text/font_atlas.hpp"
#include "graphics/text/freetype.hpp"
#include "graphics/vertex.hpp"

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
    , m_atlas()
    , m_fonts()
    , m_font_names()
    , m_window_size()
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

FontID FontManager::load_font(std::string name, std::string filepath)
{
    { // return early if the name corresponds to a known font
        auto it = m_font_names.find(name);
        if (it != std::end(m_font_names)) {
            return it->second;
        }
    }
    m_fonts.emplace_back(this, std::move(filepath), 48); // TODO: I ignore pixel size for now until valve-style fonts either work or don't
    FontID font_id = m_fonts.size();
    m_font_names.insert(std::make_pair(std::move(name), font_id));
    return font_id;
}

void FontManager::render_text(const std::string& text, ushort x, ushort y, const FontID font_id)
{
    if (font_id == INVALID_FONT) {
        log_critical << "Cannot render text \"" << text << "\" with an invalid Font";
        return;
    }

    std::vector<Vertex> vertices;
    vertices.reserve(text.size());
    const Font& font = m_fonts[font_id - 1];
    for (const auto character : text) {
        const Glyph& glyph = font.get_glyph(static_cast<codepoint_t>(character)); // TODO: text rendering will only work for pure ascii

        // skip glyphs wihout pixels
        if (glyph.rect.width && glyph.rect.height) {

            // create the quad (2*3 vertices) to render the character
            const Aabr glyph_rect(static_cast<float>(glyph.rect.x),
                                  static_cast<float>(glyph.rect.y),
                                  static_cast<float>(glyph.rect.width),
                                  static_cast<float>(glyph.rect.height));

            const Aabr quad_rect(static_cast<float>(x + glyph.left),
                                 static_cast<float>(y),
                                 static_cast<float>(x + glyph.left + glyph.rect.width),
                                 static_cast<float>(y + glyph.top - glyph.rect.height));

            vertices.emplace_back(Vertex{Vector2{quad_rect.left(), quad_rect.bottom()},
                                         Vector2{glyph_rect.left(), glyph_rect.bottom()}});
            vertices.emplace_back(Vertex{Vector2{quad_rect.right(), quad_rect.bottom()},
                                         Vector2{glyph_rect.right(), glyph_rect.bottom()}});
            vertices.emplace_back(Vertex{Vector2{quad_rect.left(), quad_rect.top()},
                                         Vector2{glyph_rect.left(), glyph_rect.top()}});

            vertices.emplace_back(Vertex{Vector2{quad_rect.right(), quad_rect.bottom()},
                                         Vector2{glyph_rect.right(), glyph_rect.bottom()}});
            vertices.emplace_back(Vertex{Vector2{quad_rect.right(), quad_rect.top()},
                                         Vector2{glyph_rect.right(), glyph_rect.top()}});
            vertices.emplace_back(Vertex{Vector2{quad_rect.left(), quad_rect.top()},
                                         Vector2{glyph_rect.left(), glyph_rect.top()}});
        }

        // advance to the next character position
        x += glyph.advance_x;
        y += glyph.advance_y;
    }

    // update the shader uniforms
    m_font_shader.use(); // TODO: Can I use the cell shader for fonts?

    const Transform3 proj_matrix = Transform3::orthographic(m_window_size.width, m_window_size.height, 0.05f, 100.0f);
    glUniformMatrix4fv(m_view_proj_matrix_uniform, 1, GL_FALSE, proj_matrix.as_ptr());

    const Transform3 world_matrix = Transform3::translation(Vector3{0, 0, -10});
    glUniformMatrix4fv(m_world_matrix_uniform, 1, GL_FALSE, world_matrix.as_ptr());

    // render the quads
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), &vertices.front(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
}

} // namespace notf
