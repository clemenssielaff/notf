#pragma once

#include <string>

#include "core/glfw_wrapper.hpp"

namespace signal {

/**
 * @brief Compiles an OpenGL shader program from files.
 *
 * @param vertex_shader_path    Path to the vertex shader file.
 * @param fragment_shader_path  Path to the fragment shader file.
 *
 * @return  OpenGL handle for the produced shader, will be zero if the compilation failed.
 */
GLuint produce_gl_program(std::string vertex_shader_path, std::string fragment_shader_path);

/*!
 * @brief Helper class to make sure that a Vertex Array Object (VAO) is always unbound after a function exits.
 */
struct VaoBindRAII {
    VaoBindRAII(GLuint vao) { glBindVertexArray(vao); }
    ~VaoBindRAII() { glBindVertexArray(0); }
};

} // namespace signal
