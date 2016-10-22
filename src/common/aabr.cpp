#include "common/aabr.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

std::ostream& operator<<(std::ostream& out, const Aabr& aabr)
{
    return out << "Rect([" << aabr.left() << ", " << aabr.bottom() << "], ["
               << aabr.right() << ", " << aabr.top() << "])";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Aabr) == sizeof(Vector2) * 2,
              "This compiler seems to inject padding bits into the notf::Aabr memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabr>::value,
              "This compiler does not recognize the notf::Aabr as a POD.");


} // namespace notf
