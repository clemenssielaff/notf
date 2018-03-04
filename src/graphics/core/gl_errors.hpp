#pragma once

#include "common/string.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

/// Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line      Line at which the error occurred.
/// @param file      File in which the error occurred.
/// @param function  Name of the function in which the error occurred.
/// @return          The number of encountered errors.
int _notf_check_gl_error(unsigned int line, const char* file, const char* function);

/// Clear all OpenGl errors that have occured since the application start or the last call to `gl_clear_error` or
/// `notf_check_gl_error`.
void _notf_clear_gl_errors();

} // namespace detail

/// Check for and return the last OpenGL error.
/// For a simple error reporting mechanism use `notf_check_gl_error` which is a noop in release.
#define notf_get_gl_error() detail::_notf_check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)

#ifdef NOTF_DEBUG
#    define notf_clear_gl_errors() detail::_notf_clear_gl_errors()
#    define notf_check_gl_error() detail::_notf_check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#    define notf_check_gl(A) A; notf_check_gl_error()
#else
#    define notf_check_gl_error() do {} while (0)
#    define notf_clear_gl_errors() do {} while (0)
#    define notf_check_gl(A) A
#endif

NOTF_CLOSE_NAMESPACE
