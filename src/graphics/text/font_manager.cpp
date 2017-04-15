#include "graphics/text/font_manager.hpp"

#include "common/aabr.hpp"
#include "common/log.hpp"
#include "common/system.hpp"
#include "common/xform3.hpp"
#include "core/opengl.hpp"
#include "graphics/gl_errors.hpp"
#include "graphics/graphics_context.hpp"
#include "graphics/text/font_atlas.hpp"
#include "graphics/text/freetype.hpp"
#include "graphics/vertex.hpp"

namespace notf {

FontManager::FontManager(GraphicsContext& graphics_context)
    : m_freetype(nullptr)
    , m_graphics_context(graphics_context)
    , m_atlas(graphics_context)
    , m_fonts()
    , m_font_names()
{
    if (FT_Init_FreeType(&m_freetype)) {
        log_critical << "Failed to initialize the Freetype library";
        return;
    }
}

FontManager::~FontManager()
{
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

void FontManager::render_atlas()
{
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, m_atlas.get_texture_id());

    //    m_font_shader->bind();

    //    GLfloat black[4] = {0, 0, 0, 1};
    //    glUniform4fv(m_color_uniform, 1, black);

    //    auto worldMatrix          = Transform3::translation(Vector3f{0, 0, -10});
    //    auto viewMatrix           = Transform3::identity();
    //    auto projectionMatrix     = Transform3::orthographic(m_window_size.width, m_window_size.height, 0.05f, 100.0f);
    //    Transform3 viewProjMatrix = projectionMatrix * viewMatrix;

    //    glUniformMatrix4fv(m_world_matrix_uniform, 1, GL_FALSE, worldMatrix.as_ptr());
    //    glUniformMatrix4fv(m_view_proj_matrix_uniform, 1, GL_FALSE, viewProjMatrix.as_ptr());

    //    float l = m_window_size.width / -2.f;
    //    float b = m_window_size.height / -2.f;

    //    GLfloat box[4][4] = {
    //        {l, b + 512, 0, 0},
    //        {l, b, 0, 1},
    //        {l + 512, b + 512, 1, 0},
    //        {l + 512, b, 1, 1},
    //    };
    //    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
    //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //    check_gl_error();
}

std::shared_ptr<Texture2> FontManager::get_atlas_texture() const
{
    return m_atlas.get_texture();
}

} // namespace notf
