#pragma once

#include <string>

namespace signal {

/*!
 * @brief Reads the contents of a file into a string.
 *
 * @param file_path Path (relative to cwd) of the file to read.
 *
 * @return Contents of the file, returns an empty string if loading failed.
 */
std::string read_file(std::string file_path);

} // namespace signal
