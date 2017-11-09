#pragma once

#include "common/string.hpp"

namespace notf {

/// @brief Checks if there was an OpenGL error and reports it to Signal's logger.
/// @param line      Line at which the error occurred.
/// @param file      File in which the error occurred.
/// @param function  Name of the function in which the error occurred.
/// @return          The number of encountered errors.
int _gl_check_error(unsigned int line, const char* file, const char* function);

/// @brief Clear all OpenGl errors that have occured since the application start or the last call to `gl_clear_error` or
/// `gl_check_error`.
void gl_clear_error();

/// @brief Check for and return the last OpenGL error.
/// For a simple error reporting mechanism use `gl_check_error` which is a noop in release.
#define gl_get_error() _gl_check_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)

#ifdef _DEBUG
#define gl_check_error() _gl_check_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#else
#define gl_check_error() \
    do {                 \
    } while (0)
#endif

} // namespace notf
