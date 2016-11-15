#include "common/size2r.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Size2r& size)
{
    return out << "Size2r(width: " << size.width << ", height: " << size.height << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Size2r) == sizeof(float) * 2,
              "This compiler seems to inject padding bits into the notf::Size2r memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2r>::value,
              "This compiler does not recognize the notf::Size2r as a POD.");

} // namespace notf
