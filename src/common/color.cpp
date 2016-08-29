#include "common/color.hpp"

#include <iostream>
#include <type_traits>

namespace signal {

std::ostream& operator<<(std::ostream& out, const Color& color)
{
    return out << "Color(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
}


/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Color) == sizeof(Real) * 4,
              "This compiler seems to inject padding bits into the signal::Color memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Color>::value,
              "This compiler does not recognize the signal::Color as a POD.");

} // namespace signal
