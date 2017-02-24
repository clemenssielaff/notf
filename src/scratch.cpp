#include <ft2build.h>
#include FT_FREETYPE_H

#include "core/glfw_wrapper.hpp"

#include "common/size2i.hpp"
#include "common/log.hpp"
#include "graphics/shader.hpp"

using namespace notf;

namespace { // anonymous
// clang-format off

const char* font_vertex_shader =
#include "shader/font.vert"

// fragment shader source
const char* font_fragment_shader =
#include "shader/font.frag"

// clang-format on
} // namespace anonymous

FT_Library ft;
FT_Face face;
FT_GlyphSlot glyph;

GLint uniform_color;
GLint uniform_tex;
GLint attribute_coord = 0;

void render_text(const char *text, float x, float y, float sx, float sy) {
  const char *p;

  for(p = text; *p; p++) {
    if(FT_Load_Char(face, *p, FT_LOAD_RENDER))
        continue;

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_R8,
      glyph->bitmap.width,
      glyph->bitmap.rows,
      0,
      GL_RED,
      GL_UNSIGNED_BYTE,
      glyph->bitmap.buffer
    );

    float x2 = x + glyph->bitmap_left * sx;
    float y2 = -y - glyph->bitmap_top * sy;
    float w = glyph->bitmap.width * sx;
    float h = glyph->bitmap.rows * sy;

    GLfloat box[4][4] = {
        {x2,     -y2    , 0, 0},
        {x2 + w, -y2    , 1, 0},
        {x2,     -y2 - h, 0, 1},
        {x2 + w, -y2 - h, 1, 1},
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    x += (glyph->advance.x/64) * sx;
    y += (glyph->advance.y/64) * sy;
  }
}


int main(void)
{
    GLFWwindow* window;

    // install the log handler first, to catch errors right away
    std::unique_ptr<LogHandler> log_handler = std::make_unique<LogHandler>(128, 200);
    install_log_message_handler(std::bind(&LogHandler::push_log, log_handler.get(), std::placeholders::_1));
    log_handler->start();

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    Shader shader = Shader::build("font_shader", font_vertex_shader, font_fragment_shader);
    if (!shader.is_valid()) {
        return -1;
    }
    shader.use();
    uniform_color = glGetUniformLocation(shader.get_id(), "color");
    uniform_tex = glGetUniformLocation(shader.get_id(), "tex");

    ///


    if(FT_Init_FreeType(&ft)) {
      fprintf(stderr, "Could not init freetype library\n");
      return 1;
    }


    if(FT_New_Face(ft, "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf", 0, &face)) {
      fprintf(stderr, "Could not open font\n");
      return 1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48);
    glyph = face->glyph;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint tex;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(uniform_tex, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glEnableVertexAttribArray(attribute_coord);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

    ///

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();

        GLfloat black[4] = {0, 0, 0, 1};
        glUniform4fv(uniform_color, 1, black);

        Size2i buffer_size;
        glfwGetWindowSize(window, &buffer_size.width, &buffer_size.height);
        float sx = 2.f / static_cast<float>(buffer_size.width);
        float sy = 2.f / static_cast<float>(buffer_size.height);

        render_text("The Quick Brown Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 50 * sy,    sx, sy);
        render_text("The Misaligned Fox Jumps Over The Lazy Dog",
                    -1 + 8.5 * sx, 1 - 100.5 * sy, sx, sy);

        FT_Set_Pixel_Sizes(face, 0, 48);
        render_text("The Small Texture Scaled Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 175 * sy,   sx * 0.5, sy * 0.5);
        FT_Set_Pixel_Sizes(face, 0, 24);
        render_text("The Small Font Sized Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 200 * sy,   sx, sy);
        FT_Set_Pixel_Sizes(face, 0, 48);
        render_text("The Tiny Texture Scaled Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 235 * sy,   sx * 0.25, sy * 0.25);
        FT_Set_Pixel_Sizes(face, 0, 12);
        render_text("The Tiny Font Sized Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 250 * sy,   sx, sy);

        FT_Set_Pixel_Sizes(face, 0, 48);
        render_text("The Solid Black Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 430 * sy,   sx, sy);

        GLfloat red[4] = {1, 0, 0, 1};
        glUniform4fv(uniform_color, 1, red);
        render_text("The Solid Red Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 330 * sy,   sx, sy);
        render_text("The Solid Red Fox Jumps Over The Lazy Dog",
                    -1 + 28 * sx,  1 - 450 * sy,   sx, sy);

        GLfloat transparent_green[4] = {0, 1, 0, 0.5};
        glUniform4fv(uniform_color, 1, transparent_green);
        render_text("The Transparent Green Fox Jumps Over The Lazy Dog",
                    -1 + 8 * sx,   1 - 380 * sy,   sx, sy);
        render_text("The Transparent Green Fox Jumps Over The Lazy Dog",
                    -1 + 18 * sx,  1 - 440 * sy,   sx, sy);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    /* Shutdown */
    log_handler->stop();
    log_handler->join();
    glfwTerminate();
    return 0;
}
