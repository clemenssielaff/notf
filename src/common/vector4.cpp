#include "common/vector4.hpp"

#include <iostream>

namespace notf {

// Vector4f ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector4f& vec)
{
    return out << "Vector4f(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}

static_assert(sizeof(Vector4f) == sizeof(float) * 4,
              "This compiler seems to inject padding bits into the notf::Vector4f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector4f>::value,
              "This compiler does not recognize notf::Vector4f as a POD.");

// Vector4d ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector4d& vec)
{
    return out << "Vector4d(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}

static_assert(sizeof(Vector4d) == sizeof(double) * 4,
              "This compiler seems to inject padding bits into the notf::Vector4d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector4f>::value,
              "This compiler does not recognize notf::Vector4d as a POD.");

// Vector4h ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector4h& vec)
{
    return out << "Vector4h(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w() << ")";
}

static_assert(sizeof(Vector4h) == sizeof(half) * 4,
              "This compiler seems to inject padding bits into the notf::Vector4h memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector4f>::value,
              "This compiler does not recognize notf::Vector4h as a POD.");

} // namespace notf
