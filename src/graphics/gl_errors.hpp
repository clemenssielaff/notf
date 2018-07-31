#pragma once

#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

/// Specialized exception that logs an error from OpenGL.
NOTF_EXCEPTION_TYPE(opengl_error);

// ================================================================================================================== //

namespace detail {

/// Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line             Line at which the error occurred.
/// @param file             File in which the error occurred.
/// @param function         Name of the function in which the error occurred.
/// @throws opengl_error    Error containing the reported OpenGl error message.
void check_gl_error(unsigned int line, const char* file, const char* function);

} // namespace detail

// ================================================================================================================== //

/// Clear all OpenGl errors that have occured since the application start or the last call to `clear_gl_errors`.
void clear_gl_errors();

// ================================================================================================================== //

#ifdef NOTF_DEBUG
#define NOTF_CHECK_GL(A) \
    (A);                 \
    detail::check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#else
#define NOTF_CHECK_GL(A) (A)
#endif

NOTF_CLOSE_NAMESPACE
