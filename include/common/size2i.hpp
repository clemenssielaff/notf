#pragma once

#include <iosfwd>

namespace notf {

struct Size2f;

/** 2D size with integer values. */
struct Size2i {

    /** Width */
    int width;

    /** Height */
    int height;

    Size2i() = default;

    Size2i(int width, int height)
        : width(width)
        , height(height)
    {
    }

    static Size2i from_size2f(const Size2f& size2f);

    /* Operators ******************************************************************************************************/

    /** Tests if this Size is valid (>=0) in both dimensions. */
    bool is_valid() const { return width >= 0 && height >= 0; }

    /** Tests if this Size is null. */
    bool is_null() const { return width == 0 && height == 0; }

    bool operator==(const Size2i& other) const { return (other.width == width && other.height == height); }

    bool operator!=(const Size2i& other) const { return (other.width != width || other.height != height); }
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Size2i into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param size  Size2i to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Size2i& size);

} // namespace notf
