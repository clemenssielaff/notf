#pragma once

#include "common/string.hpp"

namespace notf {

namespace detail {

/// Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line      Line at which the error occurred.
/// @param file      File in which the error occurred.
/// @param function  Name of the function in which the error occurred.
/// @return          The number of encountered errors.
int _gl_check_error(unsigned int line, const char* file, const char* function);

/// Clear all OpenGl errors that have occured since the application start or the last call to `gl_clear_error` or
/// `gl_check_error`.
void _gl_clear_errors();

} // namespace detail

/// Check for and return the last OpenGL error.
/// For a simple error reporting mechanism use `gl_check_error` which is a noop in release.
#define gl_get_error() detail::_gl_check_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)

#ifdef NOTF_DEBUG
#define gl_check_error() detail::_gl_check_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#define gl_clear_errors() detail::_gl_clear_errors()
#define gl_check(A) A; gl_check_error()
#else
#define gl_check_error() do {} while (0)
#define gl_clear_errors() do {} while (0)
#define gl_check(A) A
#endif

} // namespace notf
