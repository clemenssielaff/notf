#include "common/triangle.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

// Trianglef =========================================================================================================//

std::ostream& operator<<(std::ostream& out, const Trianglef& triangle)
{
    return out << "Trianglef("
               << "(" << triangle.a.x() << ", " << triangle.a.y() << "), "
               << "(" << triangle.b.x() << ", " << triangle.b.y() << "), "
               << "(" << triangle.c.x() << ", " << triangle.c.y() << "))";
}

static_assert(sizeof(Trianglef) == sizeof(Vector2f) * 3,
              "This compiler seems to inject padding bits into the notf::Trianglef memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Trianglef>::value, "This compiler does not recognize notf::Trianglef as a POD.");

} // namespace notf
