//#include <ft2build.h>
//#include FT_FREETYPE_H

//#include "core/application.hpp"
//#include "core/window.hpp"
//#include "core/render_manager.hpp"
//#include "graphics/graphics_context.hpp"

//#include "common/log.hpp"
//#include "common/size2.hpp"
//#include "common/xform3.hpp"
//#include "graphics/shader.hpp"
//#include "graphics/text/font_manager.hpp"


//using namespace notf;

//namespace { // anonymous
//// clang-format off

//const char* font_vertex_shader =
//#include "shader/font.vert"

//// fragment shader source
//const char* font_fragment_shader =
//#include "shader/font.frag"

//// clang-format on
//} // namespace anonymous

//FT_Library freetype_library;
//FT_Face face;
//FT_GlyphSlot glyph;

//GLint uniform_color;
//GLint uniform_tex;
//GLint uniform_view_proj_matrix;
//GLint uniform_world_matrix;
//GLint attribute_coord = 0;
//float canvas_width, canvas_height;

//void render_text(const char* text, const float x, const float y)
//{
//    const char* p;

//    float pencil_x = x;
//    float pencil_y = canvas_height - y;

//    for (p = text; *p; p++) {
//        if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
//            continue; /* ignore errors */

//        glTexImage2D(
//            GL_TEXTURE_2D,
//            0,
//            GL_R8,
//            glyph->bitmap.width,
//            glyph->bitmap.rows,
//            0,
//            GL_RED,
//            GL_UNSIGNED_BYTE,
//            glyph->bitmap.buffer);

//        auto worldMatrix          = Transform3::translation(Vector3f{0, 0, -10});
//        auto viewMatrix           = Transform3::identity();
//        auto projectionMatrix     = Transform3::orthographic(canvas_width, canvas_height, 0.05f, 100.0f);
//        Transform3 viewProjMatrix = projectionMatrix * viewMatrix;

//        glUniformMatrix4fv(uniform_world_matrix, 1, GL_FALSE, worldMatrix.as_ptr());
//        glUniformMatrix4fv(uniform_view_proj_matrix, 1, GL_FALSE, viewProjMatrix.as_ptr());

//        const float guad_x      = (canvas_width / -2.f) + (pencil_x + glyph->bitmap_left);
//        const float quad_y      = (canvas_height / -2.f) + (pencil_y + glyph->bitmap_top);
//        const float quad_width  = static_cast<float>(glyph->bitmap.width);
//        const float quad_height = static_cast<float>(glyph->bitmap.rows);

//        GLfloat box[4][4] = {
//            {guad_x, quad_y, 0, 0},
//            {guad_x, quad_y - quad_height, 0, 1},
//            {guad_x + quad_width, quad_y, 1, 0},
//            {guad_x + quad_width, quad_y - quad_height, 1, 1},
//        };

//        glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
//        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//        pencil_x += (glyph->advance.x / 64);
//        pencil_y += (glyph->advance.y / 64);
//    }
//}

//int main(int argc, char* argv[])
//{
//    ApplicationInfo app_info;
//    app_info.argc         = argc;
//    app_info.argv         = argv;
//    app_info.enable_vsync = false;
//    Application& app      = Application::initialize(app_info);

//    // window
//    WindowInfo window_info;
//    window_info.icon               = "notf.png";
//    window_info.size               = {800, 600};
//    window_info.clear_color        = Color("#262a32");
//    window_info.is_resizeable      = true;
//    std::shared_ptr<Window> window = Window::create(window_info);


//    if (FT_New_Face(freetype_library, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 0, &face)) {
//        fprintf(stderr, "Could not open font\n");
//        return 1;
//    }
//    FT_Set_Pixel_Sizes(face, 0, 48);
//    glyph = face->glyph;

//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    //    glEnable(GL_CULL_FACE);
//    glfwSwapInterval(1);

//    FontManager font_manager;
////    FontID font12 = font_manager.load_font("Roboto12", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 12);
////    FontID font16 = font_manager.load_font("Roboto16", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 16);
////    FontID font24 = font_manager.load_font("Roboto24", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 24);
////    FontID font32 = font_manager.load_font("Roboto32", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 32);
//    FontID font48 = font_manager.load_font("Roboto48", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 48);
//    // TODO: void notf::FontAtlas::_add_node(size_t, const notf::FontAtlas::Rect&): Assertion `rect_right < m_width' failed.

//    GLuint tex;

//    glGenTextures(1, &tex);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, tex);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//    GLuint vbo;
//    glGenBuffers(1, &vbo);
//    glEnableVertexAttribArray(attribute_coord);
//    glBindBuffer(GL_ARRAY_BUFFER, vbo);
//    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

//    glUniform1i(uniform_tex, 0);

//    ///

//    /* Loop until the user closes the window */
//    while (!glfwWindowShouldClose(window)) {

//        Size2i buffer_size;
//        glfwGetWindowSize(window, &buffer_size.width, &buffer_size.height);
//        glViewport(0, 0, buffer_size.width, buffer_size.height);

//        glClearColor(1, 1, 1, 1);
//        glClear(GL_COLOR_BUFFER_BIT);

//        GLfloat black[4] = {0, 0, 0, 1};
//        glUniform4fv(uniform_color, 1, black);

//        canvas_width  = static_cast<float>(buffer_size.width);
//        canvas_height = static_cast<float>(buffer_size.height);

//        font_manager.set_window_size(buffer_size);
//        font_manager.render_text("The Quick Brown Fox Jumps Over The Lazy Dog", 8, 50, font48);

//        font_manager.render_atlas();

//        shader.use();

//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, tex);

//        render_text("The Quick Brown Fox Jumps Over The Lazy Dog", 8, 100);

//        //        render_text("The Quick Brown Fox Jumps Over The Lazy Dog", 8, 50);
//        //        render_text("The Misaligned Fox Jumps Over The Lazy Dog", 8.5, 100.5);

//        //        FT_Set_Pixel_Sizes(face, 0, 24);
//        //        render_text("The Small Font Sized Fox Jumps Over The Lazy Dog", 8, 200);

//        //        FT_Set_Pixel_Sizes(face, 0, 12);
//        //        render_text("The Tiny Font Sized Fox Jumps Over The Lazy Dog", 8, 250);

//        //        FT_Set_Pixel_Sizes(face, 0, 48);
//        //        render_text("The Solid Black Fox Jumps Over The Lazy Dog", 8, 430);

//        //        GLfloat red[4] = {1, 0, 0, 1};
//        //        glUniform4fv(uniform_color, 1, red);
//        //        render_text("The Solid Red Fox Jumps Over The Lazy Dog", 8, 330);
//        //        render_text("The Solid Red Fox Jumps Over The Lazy Dog", 28, 450);

//        //        GLfloat transparent_green[4] = {0, 1, 0, 0.5};
//        //        glUniform4fv(uniform_color, 1, transparent_green);
//        //        render_text("The Transparent Green Fox Jumps Over The Lazy Dog", 8, 380);
//        //        render_text("The Transparent Green Fox Jumps Over The Lazy Dog", 18, 440);

//        /* Swap front and back buffers */
//        glfwSwapBuffers(window);

//        /* Poll for and process events */
//        glfwPollEvents();
//    }

//    /* Shutdown */
//    log_handler->stop();
//    log_handler->join();

//    FT_Done_FreeType(freetype_library);

//    glfwTerminate();
//    return 0;
//}
