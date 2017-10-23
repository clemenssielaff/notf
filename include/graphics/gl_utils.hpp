#pragma once

#include <stddef.h>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

struct half;

/** Modern way of creating a GLvoid buffer offset. */
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(0) + offset; }

/** Prints all available OpenGL ES system information to the log. */
void gl_log_system_info();

/** Returns the human readable name to an OpenGL type enum. */
const std::string& gl_type_name(GLenum type);

/** Returns the OpenGL type identifier for selected C++ types. */
GLenum to_gl_type(const GLbyte&);
GLenum to_gl_type(const GLubyte &);
GLenum to_gl_type(const GLshort&);
GLenum to_gl_type(const GLushort&);
GLenum to_gl_type(const GLint&);
GLenum to_gl_type(const GLuint&);
GLenum to_gl_type(const half&);
GLenum to_gl_type(const GLfloat&);

} // namespace notf
