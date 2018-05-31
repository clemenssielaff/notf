#include "common/bezier.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// Bezier2f ========================================================================================================= //

std::ostream& operator<<(std::ostream& out, const CubicBezier2f& /*bezier*/)
{
    return out << "Bezier2f(\n"; // TODO: Bezier printing
//               << "\t" << bezier.start << "\n"
//               << "\t" << bezier.ctrl1 << "\n"
//               << "\t" << bezier.ctrl2 << "\n"
//               << "\t" << bezier.end << ")";
}

static_assert(sizeof(CubicBezier2f::Segment) == sizeof(Vector2f) * 4,
              "This compiler seems to inject padding bits into the notf::Bezier2f memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<CubicBezier2f::Segment>::value, "This compiler does not recognize notf::Bezier2f as a POD.");

NOTF_CLOSE_NAMESPACE
