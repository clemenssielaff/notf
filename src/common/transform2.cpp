#include "common/transform2.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Transform2& mat)
{
    return out << "Transform2(\n"
               << "\t" << mat[0][0] << ",\t" << mat[0][1] << ",\t" << mat[0][2] << "\n"
               << "\t" << mat[1][0] << ",\t" << mat[1][1] << ",\t" << mat[1][2] << "\n"
               << "\t0,\t0,\t1\n)";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Transform2) == sizeof(Real) * 6,
              "This compiler seems to inject padding bits into the notf::Transform2 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Transform2>::value,
              "This compiler does not recognize the notf::Transform2 as a POD.");

} // namespace notf
