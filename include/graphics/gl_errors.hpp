#pragma once

namespace notf {

using uint = unsigned int;

/** Checks if there was an OpenGL error and reports it to Signal's logger.
 * @param line      Line at which the error occurred.
 * @param file      File in which the error occurred.
 * @param function  Name of the function in which the error occurred.
 * @return          The number of encountered errors.
 */
int _check_gl_error(uint line, const char* file, const char* function);

#if NOTF_LOG_LEVEL > NOTF_LOG_LEVEL_WARNING // evaluates to false if both are not defined
#define check_gl_error() _check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#else
#define check_gl_error() (0)
#endif

} // namespace notf
