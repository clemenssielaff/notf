#include "common/vector2.hpp"

#include <iostream>

namespace notf {

// Vector2f ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector2f& vec)
{
    return out << "Vector2f(" << vec.x() << ", " << vec.y() << ")";
}

static_assert(sizeof(Vector2f) == sizeof(float) * 2,
              "This compiler seems to inject padding bits into the notf::Vector2f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector2f>::value,
              "This compiler does not recognize notf::Vector2f as a POD.");

// Vector2d ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector2d& vec)
{
    return out << "Vector2d(" << vec.x() << ", " << vec.y() << ")";
}

static_assert(sizeof(Vector2d) == sizeof(double) * 2,
              "This compiler seems to inject padding bits into the notf::Vector2d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector2d>::value,
              "This compiler does not recognize notf::Vector2d as a POD.");

// Vector2i ==========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Vector2i& vec)
{
    return out << "Vector2i(" << vec.x() << ", " << vec.y() << ")";
}

static_assert(sizeof(Vector2i) == sizeof(int) * 2,
              "This compiler seems to inject padding bits into the notf::Vector2i memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Vector2i>::value,
              "This compiler does not recognize notf::Vector2i as a POD.");

} // namespace notf
