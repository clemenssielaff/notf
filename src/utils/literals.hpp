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

} // namespace literals

NOTF_CLOSE_NAMESPACE
