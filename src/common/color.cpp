#include "common/color.hpp"

#include <iostream>
#include <type_traits>

namespace { // anonymous

using notf::Real;

Real hue_to_rgb(Real h, Real m1, Real m2)
{
    if (h < 0) {
        h += 1;
    }
    else if (h > 1) {
        h -= 1;
    }

    if (h < Real(1) / Real(6)) {
        return m1 + (m2 - m1) * h * 6;
    }

    else if (h < Real(2) / Real(6)) {
        return m2;
    }
    else if (h < Real(4) / Real(6)) {
        return m1 + (m2 - m1) * (Real(2) / Real(3) - h) * 6;
    }
    return m1;
}
}

namespace notf {

Color Color::from_hsl(Real h, Real s, Real l, Real a)
{
    h = fmodf(h, 1);
    if (h < 0) {
        h += 1;
    }
    s = clamp(s, 0, 1);
    l = clamp(l, 0, 1);

    Real m2 = l <= Real(0.5) ? (l * (1 + s)) : (l + s - l * s);
    Real m1 = 2 * l - m2;

    Color result;
    result.r = clamp(hue_to_rgb(h + Real(1) / Real(3), m1, m2), 0, 1);
    result.g = clamp(hue_to_rgb(h, m1, m2), 0, 1);
    result.b = clamp(hue_to_rgb(h - Real(1) / Real(3), m1, m2), 0, 1);
    result.a = clamp(a, 0, 1);
    return result;
}

std::ostream& operator<<(std::ostream& out, const Color& color)
{
    return out << "Color(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Color) == sizeof(Real) * 4,
              "This compiler seems to inject padding bits into the notf::Color memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Color>::value,
              "This compiler does not recognize the notf::Color as a POD.");

} // namespace notf
