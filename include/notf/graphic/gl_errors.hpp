#pragma once

#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// opengl error handling ============================================================================================ //

/// Specialized exception that logs an error from OpenGL.
NOTF_EXCEPTION_TYPE(OpenGLError);

namespace detail {

/// Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line             Line at which the error occurred.
/// @param file             File in which the error occurred.
/// @throws opengl_error    Error containing the reported OpenGl error message.
void check_gl_error(unsigned int line, const char* file);

} // namespace detail

#ifdef NOTF_DEBUG
#define NOTF_CHECK_GL(A) \
    (A);                 \
    detail::check_gl_error(__LINE__, notf::filename_from_path(__FILE__))
#else
#define NOTF_CHECK_GL(A) (A)
#endif

/// Clear all OpenGl errors that have occured since the application start or the last call to `clear_gl_errors`.
void clear_gl_errors();

NOTF_CLOSE_NAMESPACE
