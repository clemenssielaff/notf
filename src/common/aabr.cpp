#include <iostream>
#include <type_traits>

#include "common/aabr.hpp"

using namespace notf;

/* Aabrf **************************************************************************************************************/

std::ostream& operator<<(std::ostream& out, const Aabrf& aabr)
{
    return out << "Aabrf([" << aabr.left() << ", " << aabr.top() << "], ["
               << aabr.right() << ", " << aabr.bottom() << "])";
}

static_assert(sizeof(Aabrf) == sizeof(Vector2f) * 2,
              "This compiler seems to inject padding bits into the notf::Aabrf memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabrf>::value,
              "This compiler does not recognize notf::Aabrf as a POD.");
