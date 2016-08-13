#include "core/shadercomponent.hpp"

#include "common/const.hpp"
#include "linmath.h"
#include "common/random.hpp"

static const struct
{
    float x, y;
    float r, g, b;
} vertices[3] = {
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    { 0.6f, -0.4f, 0.f, 1.f, 0.f },
    { 0.f, 0.6f, 0.f, 0.f, 1.f }
};
static const char* vertex_shader_text = "uniform mat4 MVP;\n"
                                        "attribute vec3 vCol;\n"
                                        "attribute vec2 vPos;\n"
                                        "varying vec3 color;\n"
                                        "void main()\n"
                                        "{\n"
                                        "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
                                        "    color = vCol;\n"
                                        "}\n";
static const char* fragment_shader_text = "varying vec3 color;\n"
                                          "void main()\n"
                                          "{\n"
                                          "    gl_FragColor = vec4(color, 1.0);\n"
                                          "}\n";

namespace signal {

ShaderComponent::ShaderComponent()
{
    test_offset = random().uniform(0.f, static_cast<float>(TWO_PI));

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
        sizeof(float) * 5, (void*)0);
    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(float) * 5, (void*)(sizeof(float) * 2));
}

void ShaderComponent::update()
{
    float ratio;
    int width = 400;
    int height = 400;
    mat4x4 model_matrix, projection_matrix, model_view_projection;
    ratio = width / static_cast<float>(height);
    mat4x4_identity(model_matrix);
    mat4x4_rotate_Z(model_matrix, model_matrix, static_cast<float>(glfwGetTime()) + test_offset);
    mat4x4_ortho(projection_matrix, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(model_view_projection, projection_matrix, model_matrix);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)model_view_projection);
    glUseProgram(program);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
}
