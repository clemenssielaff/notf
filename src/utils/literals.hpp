#pragma once

#include <string>

#define DEG_TO_RAD 0.017453292519943295769236907684886127134428718885417254560L

NOTF_OPEN_NAMESPACE
namespace literals {

/** Floating point literal to convert degrees to radians. */
constexpr long double operator"" _deg(long double deg) { return deg * DEG_TO_RAD; }

/** Integer literal to convert degrees to radians. */
constexpr long double operator"" _deg(unsigned long long int deg) { return static_cast<long double>(deg) * DEG_TO_RAD; }

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
    std::size_t found  = result.find_first_of(wrong);
    while (found != std::string::npos) {
        result[found] = right;
        found         = result.find_first_of(wrong, found + 1);
    }
    return result;
}

} // namespace literals
NOTF_CLOSE_NAMESPACE

#undef DEG_TO_RAD
