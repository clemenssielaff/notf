#pragma once

#include <stddef.h>
#include <string>

#include "graphics/forwards.hpp"

NOTF_OPEN_NAMESPACE

struct half;

/// Checks if there is a valid OpenGL context.
bool gl_is_initialized();

/// Modern way of creating a GLvoid buffer offset.
constexpr char* gl_buffer_offset(size_t offset) { return static_cast<char*>(nullptr) + offset; }

/// Prints all available OpenGL ES system information to the log.
void gl_log_system_info();

/// Returns the human readable name to an OpenGL type enum.
const std::string& gl_type_name(GLenum type);

/// Returns the OpenGL type identifier for selected C++ types.
GLenum to_gl_type(const GLbyte&);
GLenum to_gl_type(const GLubyte&);
GLenum to_gl_type(const GLshort&);
GLenum to_gl_type(const GLushort&);
GLenum to_gl_type(const GLint&);
GLenum to_gl_type(const GLuint&);
GLenum to_gl_type(const half&);
GLenum to_gl_type(const GLfloat&);

// ================================================================================================================== //

/// RAII guard for vector array object bindings.
struct VaoBindGuard final {

    // fields ------------------------------------------------------------------------------------------------------- //
    /// Vertex array object ID.
    const GLuint m_vao;

    // methods ------------------------------------------------------------------------------------------------------ //
    /// Constructor.
    /// @param vao  Vertex array object ID.
    VaoBindGuard(GLuint vao);

    /// Destructor.
    ~VaoBindGuard();
};

NOTF_CLOSE_NAMESPACE
