#include "common/vector3.hpp"

#include <iostream>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Vector3& vec)
{
    return out << "Vector3(" << vec.x << ", " << vec.y <<  ", " << vec.z << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Vector3) == sizeof(float) * 3,
              "This compiler seems to inject padding bits into the notf::Vector3 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector3>::value,
              "This compiler does not recognize notf::Vector3 as a POD.");

} // namespace notf
