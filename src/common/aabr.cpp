#include <iostream>
#include <type_traits>

#include "common/aabr.hpp"

namespace notf {

// Aabrf =============================================================================================================//

std::ostream& operator<<(std::ostream& out, const Aabrf& aabr)
{
    return out << "Aabrf(pos: [" << aabr.left() << ", " << aabr.bottom() << "], h:" << aabr.get_height()
               << ", w:" << aabr.get_width() << "])";
}

static_assert(sizeof(Aabrf) == sizeof(Vector2f) * 2,
              "This compiler seems to inject padding bits into the notf::Aabrf memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabrf>::value, "This compiler does not recognize notf::Aabrf as a POD.");

// Aabrd =============================================================================================================//

std::ostream& operator<<(std::ostream& out, const Aabrd& aabr)
{
    return out << "Aabrd(pos: [" << aabr.left() << ", " << aabr.bottom() << "], h:" << aabr.get_height()
               << ", w:" << aabr.get_width() << "])";
}

static_assert(sizeof(Aabrd) == sizeof(Vector2d) * 2,
              "This compiler seems to inject padding bits into the notf::Aabrd memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabrd>::value, "This compiler does not recognize notf::Aabrd as a POD.");

// Aabri =============================================================================================================//

std::ostream& operator<<(std::ostream& out, const Aabri& aabr)
{
    return out << "Aabri(pos: [" << aabr.left() << ", " << aabr.bottom() << "], h:" << aabr.get_height()
               << ", w:" << aabr.get_width() << "])";
}

static_assert(sizeof(Aabri) == sizeof(Vector2i) * 2,
              "This compiler seems to inject padding bits into the notf::Aabri memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Aabri>::value, "This compiler does not recognize notf::Aabri as a POD.");

} // namespace notf
