#include <iostream>
#include <type_traits>

#include "common/aabr.hpp"

namespace notf {

/* Aabrf **************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const notf::_Aabr<Vector2f>& aabr)
{
    return out << "Aabrf(pos: [" << aabr.left() << ", " << aabr.bottom() << "], h:"
               << aabr.get_height() << ", w:" << aabr.get_width() << "])";
}

static_assert(sizeof(Aabrf) == sizeof(Vector2f) * 2,
              "This compiler seems to inject padding bits into the notf::Aabrf memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabrf>::value,
              "This compiler does not recognize notf::Aabrf as a POD.");

} // namespace notf
