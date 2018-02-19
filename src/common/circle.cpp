#include <iostream>
#include <type_traits>

#include "common/circle.hpp"

namespace notf {

/* Circlef ************************************************************************************************************/

std::ostream& operator<<(std::ostream& out, const Circlef& circle)
{
    return out << "Circlef([" << circle.center.x() << ", " << circle.center.y() << "], " << circle.radius << ")";
}

static_assert(sizeof(Circlef) == sizeof(Vector2f) + sizeof(float),
              "This compiler seems to inject padding bits into the notf::Circlef memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Circlef>::value, "This compiler does not recognize notf::Circlef as a POD.");

} // namespace notf
