#pragma once

#include <iosfwd>

#include "common/float_utils.hpp"
#include "common/size2i.hpp"

namespace notf {

struct Size2i;

/** 2D size with floating point values. */
struct Size2f {

    /** Width */
    float width;

    /** Height */
    float height;

    Size2f() = default;

    Size2f(float width, float height)
        : width(width)
        , height(height)
    {
    }

    static Size2f from_size2i(const Size2i& size2i)
    {
        return {
            static_cast<float>(size2i.width),
            static_cast<float>(size2i.height)};
    }

    /* Operators ******************************************************************************************************/

    /** Tests if this Size is valid (>=0) in both dimensions. */
    bool is_valid() const { return width >= 0.f && height >= 0.f; } // !(NAN >= 0.f)

    /** Tests if a rectangle of this Size had zero area. */
    bool is_zero() const { return width == 0.f || height == 0.f; }

    bool operator==(const Size2f& other) const { return other.width == approx(width) && other.height == approx(height); }

    bool operator!=(const Size2f& other) const { return other.width != approx(width) || other.height != approx(height); }

    Size2f operator*(const float factor) const { return {width * factor, height * factor}; }

    Size2f operator/(const float divisor) const { return {width / divisor, height / divisor}; }

    const float* as_float_ptr() const { return &width; }
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Size2f into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param size  Size2f to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Size2f& size);

} // namespace notf
