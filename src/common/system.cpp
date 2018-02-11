#include "common/system.hpp"

#include <fstream>
#include <sstream>

#include "common/exception.hpp"

namespace notf {

std::string load_file(const std::string& file_path)
{
    std::ifstream file(file_path.c_str(), std::ifstream::in);
    if (!file.is_open()) {
        notf_throw_format(runtime_error, "Could not read file \"" << file_path << "\". File does not exist.");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace notf
