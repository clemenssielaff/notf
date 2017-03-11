#include "common/vector3.hpp"

#include <iostream>

namespace notf {

/* Vector3f ***********************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Vector3f& vec)
{
    return out << "Vector3f(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}

static_assert(sizeof(Vector3f) == sizeof(float) * 3,
              "This compiler seems to inject padding bits into the notf::Vector3f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector3f>::value,
              "This compiler does not recognize notf::Vector3f as a POD.");

/* Vector3d ***********************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Vector3d& vec)
{
    return out << "Vector3d(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}

static_assert(sizeof(Vector3d) == sizeof(double) * 3,
              "This compiler seems to inject padding bits into the notf::Vector3d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector3f>::value,
              "This compiler does not recognize notf::Vector3d as a POD.");

} // namespace notf