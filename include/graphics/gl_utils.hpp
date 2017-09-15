#pragma once

#include <stddef.h>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

/** Modern way of creating a GLvoid buffer offset. */
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(0) + offset; }

/** Prints all available OpenGL ES system information to the log. */
void gl_log_system_info();

/** Returns the human readable name to an OpenGL type enum. */
const std::string& gl_type_name(GLenum type);

} // namespace notf
