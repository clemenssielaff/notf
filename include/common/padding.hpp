#pragma once

#include <iosfwd>

#include "common/hash_utils.hpp"

namespace notf {

/** 4-sided Padding to use in Layouts.
 * Uses the same order as css margins, that is starting at top and then clockwise (top / right / bottom/ left).
 */
struct Padding {

    /** Top padding. */
    float top;

    /** Right padding. */
    float right;

    /** Bottom padding. */
    float bottom;

    /** Left padding. */
    float left;

    /** Even padding on all sides. */
    static Padding all(const float padding) { return {padding, padding, padding, padding}; }

    /** No padding. */
    static Padding none() { return {0.f, 0.f, 0.f, 0.f}; }

    /** Horizontal padding, sets both `left` and `right`. */
    static Padding horizontal(const float padding) { return {0.f, padding, 0.f, padding}; }

    /** Vertical padding, sets both `top` and `bottom`. */
    static Padding vertical(const float padding) { return {padding, 0.f, padding, 0.f}; }

    /** Checks if any of the sides is padding. */
    bool is_padding() const { return !(top == 0.f && right == 0.f && bottom == 0.f && left == 0.f); }

    /** Checks if this Padding is valid (all sides have values >= 0). */
    bool is_valid() const { return top >= 0.f && right >= 0.f && bottom >= 0.f && left >= 0.f; }
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Padding into a std::ostream.
 * @param out       Output stream, implicitly passed with the << operator.
 * @param padding   Padding to print.
 * @return          Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Padding& padding);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Padding. */
template <>
struct hash<notf::Padding> {
    size_t operator()(const notf::Padding& padding) const { return notf::hash(padding.top, padding.right, padding.bottom, padding.left); }
};
}