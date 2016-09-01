#include "common/size2.hpp"

#include <iostream>
#include <type_traits>

namespace signal {

std::ostream& operator<<(std::ostream& out, const Size2& size)
{
    return out << "Size2(width: " << size.width << ", height: " << size.height << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Size2) == sizeof(uint) * 2,
              "This compiler seems to inject padding bits into the signal::Size2 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2>::value,
              "This compiler does not recognize the signal::Size2 as a POD.");

} // namespace signal
