#include "common/system.hpp"

#include <fstream>
#include <sstream>

#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

std::string load_file(const std::string& file_path)
{
    std::ifstream file(file_path.c_str(), std::ifstream::in);
    if (!file.is_open()) {
        notf_throw(runtime_error, "Could not read file \"{}\". File does not exist.", file_path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

NOTF_CLOSE_NAMESPACE
