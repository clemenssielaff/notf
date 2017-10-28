#pragma once

#include <limits.h>
#include <string>

namespace notf {

/** Reads the contents of a file into a string.
 * @param file_path             Path (relative to cwd) of the file to read.
 * @throws std::runtime_error   If the file failed to load.
 * @return                      Contents of the file.
 */
std::string load_file(const std::string& file_path);

/** Like sizeof, but a returns the size of the type in bits not bytes. */
template<typename T>
constexpr size_t bitsizeof() {
	return sizeof(T) * CHAR_BIT;
}

} // namespace notf
