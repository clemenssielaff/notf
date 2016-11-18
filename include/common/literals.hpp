#pragma once

#define DEG_TO_RAD 0.017453292519943295769236907684886127134428718885417254560L

namespace notf {
namespace literals {

/** Floating point literal to convert degrees to radians. */
constexpr long double operator"" _deg (long double deg){return deg * DEG_TO_RAD;}

/** Integer literal to convert degrees to radians. */
constexpr long double operator"" _deg (unsigned long long int deg){return static_cast<long double>(deg) * DEG_TO_RAD;}

} // namespace notf::literals
} // namespace notf

#undef DEG_TO_RAD
