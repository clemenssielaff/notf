#include "common/size2f.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Size2f& size)
{
    return out << "Size2f(width: " << size.width << ", height: " << size.height << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Size2f) == sizeof(float) * 2,
              "This compiler seems to inject padding bits into the notf::Size2f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Size2f>::value,
              "This compiler does not recognize the notf::Size2f as a POD.");

} // namespace notf
