#pragma once

#include <string>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

namespace literals {

namespace detail {
inline constexpr auto deg_to_rad() { return 0.017453292519943295769236907684886127134428718885417254560L; }
} // namespace detail

/// Floating point literal to convert degrees to radians.
constexpr long double operator"" _deg(long double deg) { return deg * detail::deg_to_rad(); }

/// Integer literal to convert degrees to radians.
constexpr long double operator"" _deg(unsigned long long int deg)
{
    return static_cast<long double>(deg) * detail::deg_to_rad();
}

/// String literal for os-aware paths.
std::string operator"" _path(const char* input, size_t)
{
#ifdef NOTF_LINUX
    static const char wrong = '\\';
    static const char right = '/';
#else
#    ifdef NOTF_WINDOWS
    static const char wrong = '/';
    static const char right = '\\';
#    endif
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
