#pragma once

#include <fstream>
#include <sstream>

#include "common/log.hpp"

namespace signal {

/*!
 * @brief Reads the contents of a file into a string.
 *
 * @param file_path Path (relative to cwd) of the file to read.
 *
 * @return Contents of the file, returns an empty string if loading failed.
 */
inline std::string read_file(std::string file_path)
{
    std::ifstream file(file_path.c_str(), std::ifstream::in);
    if(!file.is_open()) {
        log_critical << "Could not read file '" << file_path << "'. File does not exist.";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace signal
