#pragma once

#include <string>

namespace notf {

/** String literal for os-aware paths. */
std::string operator"" _path(const char* input, size_t)
{
#ifdef __linux__
    static const char wrong = '\\';
    static const char right = '/';
#elif _WIN32
    static const char wrong = '/';
    static const char right = '\\';
#else
    static_assert(false, "Unknown operating system detected");
#endif
    std::string result = input;
    std::size_t found = result.find_first_of(wrong);
    while (found != std::string::npos) {
        result[found] = right;
        found = result.find_first_of(wrong, found + 1);
    }
    return result;
}

} // namespace notf
