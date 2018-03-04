#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/meta.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// Two-dimensional size.
template<typename value_t, ENABLE_IF_ARITHMETIC(value_t)>
struct Size2 {

    // fields --------------------------------------------------------------------------------------------------------//
    /// Width.
    value_t width;

    /// Height.
    value_t height;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Default (non-initializing) constructor so this struct remains a POD.
    Size2() = default;

    /// Value constructor.
    Size2(const value_t width, const value_t height) : width(width), height(height) {}

    /// Automatic type conversions between integer and real sizes.
    template<typename Other_t, ENABLE_IF_ARITHMETIC(Other_t)>
    Size2(const Size2<Other_t>& other)
        : width(static_cast<value_t>(other.width)), height(static_cast<value_t>(other.height))
    {}

    /// Creates and returns an invalid Size2 instance.
    static Size2 invalid() { return {-1, -1}; }

    /// Creates and returns zero Size2 instance.
    static Size2 zero() { return {0, 0}; }

    /// The "most wrong" Size2 (maximal negative area).
    /// Is useful as the starting point for defining the union of multiple Size2.
    static Size2 wrongest() { return {std::numeric_limits<value_t>::lowest(), std::numeric_limits<value_t>::lowest()}; }

    /// Tests if this Size is valid (>=0) in both dimensions.
    bool is_valid() const { return width >= 0 && height >= 0; }

    /// Tests if a rectangle of this Size has zero area.
    bool is_zero() const { return abs(width) <= precision_high<value_t>() && abs(height) <= precision_high<value_t>(); }

    /// Checks if the size has the same height and width.
    bool is_square() const { return (abs(width) - abs(height)) <= precision_high<value_t>(); }

    /// Returns the area of a rectangle of this Size.
    /// Always returns 0, if the size is invalid.
    value_t area() const { return max(0, width * height); }

    /// Pointer to the first element of the Size2.
    const value_t* as_ptr() const { return &width; }

    /// Tests whether two Size2s are equal.
    bool operator==(const Size2& other) const
    {
        return abs(other.width - width) <= precision_high<value_t>()
               && abs(other.height - height) <= precision_high<value_t>();
    }

    /// Tests whether two not Size2s are equal.
    bool operator!=(const Size2& other) const
    {
        return abs(other.width - width) > precision_high<value_t>()
               || abs(other.height - height) > precision_high<value_t>();
    }

    /// Returns True, if other and self are approximately the same size.
    /// @param other Size to test against.*@param epsilon Maximal allowed distance between any of the two sides.
    bool is_approx(const Size2& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return abs(other.width - width) <= epsilon && abs(other.height - height) <= epsilon;
    }

    /// Changes the Size2 by a given factor.
    Size2 operator*(const value_t factor) const { return {width * factor, height * factor}; }

    /// Changes the Size2 by a given divisor.
    Size2 operator/(const value_t divisor) const
    {
        if (divisor == 0) {
            notf_throw(logic_error, "Division by zero");
        }
        return {width / divisor, height / divisor};
    }

    /// Changes the Size2 by adding another.
    Size2 operator+(const Size2& other) const { return {width + other.width, height + other.width}; }

    /// Adds another Size2 in-place.
    Size2 operator+=(const Size2& other)
    {
        width += other.width;
        height += other.height;
        return *this;
    }

    /// Changes this Size2 to the maximum width and height of this and other.
    Size2& maxed(const Size2& other)
    {
        width  = max(width, other.width);
        height = max(height, other.height);
        return *this;
    }
};

} // namespace detail

//====================================================================================================================//

using Size2f = detail::Size2<float>;
using Size2i = detail::Size2<int>;
using Size2s = detail::Size2<short>;

// Free Functions ====================================================================================================//

/// Prints the contents of this size into a std::ostream.
/// @param out   Output stream, implicitly passed with the << operator.
/// @param size  Size2 to print.
/// @return      Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Size2f& size);
std::ostream& operator<<(std::ostream& out, const Size2i& size);
std::ostream& operator<<(std::ostream& out, const Size2s& size);

} // namespace notf

// std::hash =========================================================================================================//

namespace std {

/// std::hash specialization for notf::_RealVector2.
template<typename value_t>
struct hash<notf::detail::Size2<value_t>> {
    size_t operator()(const notf::detail::Size2<value_t>& size2) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::SIZE), size2.width, size2.height);
    }
};

} // namespace std
