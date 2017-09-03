#include "common/system.hpp"

#include <fstream>
#include <sstream>

#include "common/exception.hpp"
#include "common/log.hpp"
#include "common/string.hpp"

namespace notf {

std::string load_file(const std::string& file_path)
{
    std::ifstream file(file_path.c_str(), std::ifstream::in);
    if (!file.is_open()) {
        throw_runtime_error(string_format(
            "Could not read file \"%s\". File does not exist.", file_path.c_str()));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace notf
