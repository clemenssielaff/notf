#pragma once

#include <string>

#include "notf/meta/half.hpp"

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// gl utils ========================================================================================================= //

/// Modern way of creating a GLvoid buffer offset.
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(nullptr) + offset; }

/// Prints all available OpenGL ES system information to the log.
void gl_log_system_info();

/// Returns the human readable name to an OpenGL type enum.
/// @param type     OpenGL type enum value.
const std::string& gl_type_name(GLenum type);

/// Returns the OpenGL type identifier for selected C++ types.
inline constexpr GLenum to_gl_type(const GLbyte&) { return GL_BYTE; }
inline constexpr GLenum to_gl_type(const GLubyte&) { return GL_UNSIGNED_BYTE; }
inline constexpr GLenum to_gl_type(const GLshort&) { return GL_SHORT; }
inline constexpr GLenum to_gl_type(const GLushort&) { return GL_UNSIGNED_SHORT; }
inline constexpr GLenum to_gl_type(const GLint&) { return GL_INT; }
inline constexpr GLenum to_gl_type(const GLuint&) { return GL_UNSIGNED_INT; }
inline constexpr GLenum to_gl_type(const half&) { return GL_HALF_FLOAT; }
inline constexpr GLenum to_gl_type(const GLfloat&) { return GL_FLOAT; }

/// The expected usage of the data.
enum class GLUsage {
    DYNAMIC_DRAW, ///< Written many times, read many times by the GPU
    DYNAMIC_READ, ///< Written many times, read many times from the application
    DYNAMIC_COPY, ///< Written many times, read many times from the application as a source for new writes
    STATIC_DRAW,  ///< Written once, read many times from the GPU
    STATIC_READ,  ///< Written once, read many times from the application
    STATIC_COPY,  ///< Written once, read many times from the application as a source for new writes
    STREAM_DRAW,  ///< Written once, read only a few times by the GPU
    STREAM_READ,  ///< Written once, read only a few times from the application
    STREAM_COPY,  ///< Written once, read only a few times from the application as a source for new writes
    DEFAULT = DYNAMIC_DRAW,
};
/// Converts the GLUsage type into an OpenGL enum.
GLenum get_gl_usage(const GLUsage usage);

NOTF_CLOSE_NAMESPACE
