#include <iostream>
#include <type_traits>

#include "common/circle.hpp"

namespace notf {

std::ostream& operator<<(std::ostream& out, const Circle& circle)
{
    return out << "Circle([" << circle.center.x << ", " << circle.center.y << "], " << circle.radius << ")";
}

/*
 * Compile-time sanity check.
 */
static_assert(sizeof(Circle) == sizeof(Vector2f) + sizeof(float),
              "This compiler seems to inject padding bits into the notf::Aabr memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Circle>::value,
              "This compiler does not recognize notf::Aabr as a POD.");


} // namespace notf
