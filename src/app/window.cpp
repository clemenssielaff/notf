#include "app/window.hpp"

#include <iostream>

#include "common/devel.hpp"





#include "linmath.h"
static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    {  0.6f, -0.4f, 0.f, 1.f, 0.f },
    {   0.f,  0.6f, 0.f, 0.f, 1.f }
};
static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec3 vCol;\n"
"attribute vec2 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";
static const char* fragment_shader_text =
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";


namespace  {
static GLuint vertex_buffer, vertex_shader, fragment_shader, program;
static GLint mvp_location, vpos_location, vcol_location;

void HACK () {
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(vertex_shader);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(fragment_shader);
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    mvp_location = glGetUniformLocation(program, "MVP");
    vpos_location = glGetAttribLocation(program, "vPos");
    vcol_location = glGetAttribLocation(program, "vCol");
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 5, (void*) 0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 5, (void*) (sizeof(float) * 2));
}
}



namespace untitled {

Window::Window(const WindowInfo& info)
    : m_glfw_window(nullptr) // set in constructor
    , m_title(info.title)
{
    // set context variables before creating the window
    if (info.opengl_version_major >= 0) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, info.opengl_version_major);
    }
    if (info.opengl_version_minor >= 0) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, info.opengl_version_minor);
    }
    //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // create the window
    // might fail, but if it does this Window instance is immediately destroyed by the Application
    m_glfw_window = glfwCreateWindow(info.width, info.height, m_title.c_str(), nullptr, nullptr);

//    HACK();
}

Window::~Window()
{
    std::cout << "Closing Window \"" << m_title << "\"" << std::endl;

    // destroy the GLFW window
    if (m_glfw_window) {
        glfwDestroyWindow(m_glfw_window);
        m_glfw_window = nullptr;
    }
}

void Window::update()
{
//    float ratio;
//    int width, height;
//    mat4x4 m, p, mvp;
//    glfwGetFramebufferSize(m_glfw_window, &width, &height);
//    ratio = width / (float) height;
//    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
//    mat4x4_identity(m);
//    mat4x4_rotate_Z(m, m, (float) glfwGetTime());
//    mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
//    mat4x4_mul(mvp, p, m);
//    glUseProgram(program);
//    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
//    glDrawArrays(GL_TRIANGLES, 0, 3);
//    glfwSwapBuffers(m_glfw_window);

    /* Swap front and back buffers */
    glfwSwapBuffers(m_glfw_window);
}

void Window::on_token_key(KEY key, KEY_ACTION action, KEY_MODS mods)
{
    UNUSED(mods);

    if (key == KEY_ESCAPE && action == KEY_PRESS) {
        close();
    }
}

} // namespace untitled
