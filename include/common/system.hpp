#pragma once

#include <string>

namespace notf {

/** Reads the contents of a file into a string.
 * @param file_path     Path (relative to cwd) of the file to read.
 * @return              Contents of the file, returns an empty string if loading failed.
 */
std::string load_file(const std::string& file_path);

} // namespace notf
