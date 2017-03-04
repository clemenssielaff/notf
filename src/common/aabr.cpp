#include <iostream>
#include <type_traits>

#include "common/aabr.hpp"

namespace notf {

std::ostream& operator<<(std::ostream& out, const Aabr& aabr)
{
    return out << "Aabr([" << aabr.left() << ", " << aabr.top() << "], ["
               << aabr.right() << ", " << aabr.bottom() << "])";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Aabr) == sizeof(Vector2f) * 2,
              "This compiler seems to inject padding bits into the notf::Aabr memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabr>::value,
              "This compiler does not recognize notf::Aabr as a POD.");


} // namespace notf
