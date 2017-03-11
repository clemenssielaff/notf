#include "graphics/text/font_manager.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/system.hpp"
#include "common/xform3.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
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
    , m_color(Color(0.f, 0.f, 0.f, 1.f))
    , m_vbo(0)
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
    m_font_shader.use();

    m_color_uniform            = glGetUniformLocation(m_font_shader.get_id(), "color");
    m_texture_id_uniform       = glGetUniformLocation(m_font_shader.get_id(), "tex");
    m_view_proj_matrix_uniform = glGetUniformLocation(m_font_shader.get_id(), "view_proj_matrix");
    m_world_matrix_uniform     = glGetUniformLocation(m_font_shader.get_id(), "world_matrix");

    glUniform1i(m_texture_id_uniform, 0);

    glGenBuffers(1, &m_vbo);

    check_gl_error();
}

FontManager::~FontManager()
{
    glDeleteBuffers(1, &m_vbo);
    FT_Done_FreeType(m_freetype);
}

FontID FontManager::load_font(std::string name, std::string filepath, ushort pixel_size)
{
    { // return early if the name corresponds to a known font
        auto it = m_font_names.find(name);
        if (it != std::end(m_font_names)) { // TODO: we need a name + size combo to identify a font!
            return it->second;
        }
    }
    m_fonts.emplace_back(this, m_fonts.size(), name, std::move(filepath), pixel_size);
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

    y = m_window_size.height - y;

    static std::vector<Vertex> vertices;
    vertices.clear();
    vertices.reserve(text.size());
    const Font& font = m_fonts[font_id - 1];
    for (const auto character : text) {
        const Glyph& glyph = font.get_glyph(static_cast<codepoint_t>(character)); // TODO: text rendering will only work for pure ascii

        // skip glyphs wihout pixels
        if (glyph.rect.width && glyph.rect.height) {

            Aabrf glyph_rect(static_cast<float>(glyph.rect.x),
                            static_cast<float>(glyph.rect.y),
                            static_cast<float>(glyph.rect.width),
                            static_cast<float>(glyph.rect.height));
            glyph_rect = glyph_rect * (1 / 512.f); // TODO: MAGIC NUMBER!!!!!!!!!!!!!!

            Aabrf quad_rect(
                (m_window_size.width / -2.f) + (x + glyph.left),
                (m_window_size.height / -2.f) + (y + glyph.top),
                static_cast<float>(glyph.rect.width),
                static_cast<float>(glyph.rect.height));
            quad_rect.move_by({0, static_cast<float>(-glyph.rect.height)});

            // create the quad (2*3 vertices) to render the character
            vertices.emplace_back(Vertex{Vector2f{quad_rect.left(), quad_rect.bottom()},
                                         Vector2f{glyph_rect.left(), glyph_rect.top()}});
            vertices.emplace_back(Vertex{Vector2f{quad_rect.left(), quad_rect.top()},
                                         Vector2f{glyph_rect.left(), glyph_rect.bottom()}});
            vertices.emplace_back(Vertex{Vector2f{quad_rect.right(), quad_rect.bottom()},
                                         Vector2f{glyph_rect.right(), glyph_rect.top()}});

            vertices.emplace_back(Vertex{Vector2f{quad_rect.right(), quad_rect.bottom()},
                                         Vector2f{glyph_rect.right(), glyph_rect.top()}});
            vertices.emplace_back(Vertex{Vector2f{quad_rect.left(), quad_rect.top()},
                                         Vector2f{glyph_rect.left(), glyph_rect.bottom()}});
            vertices.emplace_back(Vertex{Vector2f{quad_rect.right(), quad_rect.top()},
                                         Vector2f{glyph_rect.right(), glyph_rect.bottom()}});
        }

        // advance to the next character position
        x += glyph.advance_x;
        y += glyph.advance_y;
    }

    // update the shader uniforms
    m_font_shader.use(); // TODO: Can I use the cell shader for fonts?

    const Transform3 proj_matrix = Transform3::orthographic(m_window_size.width, m_window_size.height, 0.05f, 100.0f);
    glUniformMatrix4fv(m_view_proj_matrix_uniform, 1, GL_FALSE, proj_matrix.as_ptr());

    const Transform3 world_matrix = Transform3::translation(Vector3f{0, 0, -10});
    glUniformMatrix4fv(m_world_matrix_uniform, 1, GL_FALSE, world_matrix.as_ptr());

    glUniform4fv(m_color_uniform, 1, m_color.as_ptr());

    GLfloat black[4] = {0, 0, 0, 1};
    glUniform4fv(m_color_uniform, 1, black);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    glBindTexture(GL_TEXTURE_2D, m_atlas.get_texture_id());
    glUniform1i(m_texture_id_uniform, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), &vertices.front(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

    //    glDisableVertexAttribArray(0);

    check_gl_error();
}

void FontManager::render_atlas() // TODO: MAGIC NUMBERS!!!!!!!!!!!!!!
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_atlas.get_texture_id());

    m_font_shader.use();

    GLfloat black[4] = {0, 0, 0, 1};
    glUniform4fv(m_color_uniform, 1, black);

    auto worldMatrix          = Transform3::translation(Vector3f{0, 0, -10});
    auto viewMatrix           = Transform3::identity();
    auto projectionMatrix     = Transform3::orthographic(m_window_size.width, m_window_size.height, 0.05f, 100.0f);
    Transform3 viewProjMatrix = projectionMatrix * viewMatrix;

    glUniformMatrix4fv(m_world_matrix_uniform, 1, GL_FALSE, worldMatrix.as_ptr());
    glUniformMatrix4fv(m_view_proj_matrix_uniform, 1, GL_FALSE, viewProjMatrix.as_ptr());

    float l = m_window_size.width / -2.f;
    float b = m_window_size.height / -2.f;

    GLfloat box[4][4] = {
        {l, b + 512, 0, 0},
        {l, b, 0, 1},
        {l + 512, b + 512, 1, 0},
        {l + 512, b, 1, 1},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    check_gl_error();
}

} // namespace notf
