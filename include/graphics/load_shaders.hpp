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

} // namespace signal
