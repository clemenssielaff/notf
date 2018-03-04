#include <iostream>
#include <type_traits>

#include "common/padding.hpp"

NOTF_OPEN_NAMESPACE

std::ostream& operator<<(std::ostream& out, const Padding& padding)
{
    return out << "Padding(" << padding.top << ", " << padding.right << ", " << padding.bottom << ", " << padding.left
               << ")";
}

/*
 * Compile-time sanity check.
 */
static_assert(sizeof(Padding) == sizeof(float) * 4,
              "This compiler seems to inject padding bits into the notf::Padding memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Padding>::value, "This compiler does not recognize notf::Padding as a POD.");

NOTF_CLOSE_NAMESPACE
