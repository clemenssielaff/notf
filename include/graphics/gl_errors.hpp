#pragma once

namespace notf {

/** Checks if there was an OpenGL error and reports it to Signal's logger.
 * @param line      Line at which the error occurred.
 * @param file      File in which the error occurred.
 * @param function  Name of the function in which the error occurred.
 * @return          The number of encountered errors.
 */
int _check_gl_error(unsigned int line, const char* file, const char* function);

/** Function forcing a check for OpenGL errors, which allows the caller to use the returned value.
 * For a simple error reporting mechanism use `check_gl_error` which is a noop in release.
 */
#define get_gl_error() _check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)

#ifdef _DEBUG
#define check_gl_error() _check_gl_error(__LINE__, notf::basename(__FILE__), __FUNCTION__)
#else
#define check_gl_error() \
	do {                 \
	} while (0)
#endif

} // namespace notf
