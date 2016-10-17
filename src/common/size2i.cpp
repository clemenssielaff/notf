#include "common/size2i.hpp"

#include <iostream>
#include <type_traits>

namespace signal {

std::ostream& operator<<(std::ostream& out, const Size2i& size)
{
    return out << "Size2i(width: " << size.width << ", height: " << size.height << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Size2i) == sizeof(uint) * 2,
              "This compiler seems to inject padding bits into the signal::Size2i memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2i>::value,
              "This compiler does not recognize the signal::Size2i as a POD.");

} // namespace signal
