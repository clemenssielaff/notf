#pragma once

#include <stddef.h>

namespace notf {

/** Modern way of creating a GLvoid buffer offset. */
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(0) + offset; }

/** Prints all available OpenGL ES system information to the log. */
void gl_log_system_info();

} // namespace notf
