#include "common/vector2.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Vector2& vec)
{
    return out << "Vector2(" << vec.x << ", " << vec.y << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Vector2) == sizeof(Real) * 2,
              "This compiler seems to inject padding bits into the notf::Vector2 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector2>::value,
              "This compiler does not recognize the notf::Vector2 as a POD.");

} // namespace notf
