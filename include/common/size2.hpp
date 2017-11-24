#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/meta.hpp"

namespace notf {

//====================================================================================================================//

/// @brief Two-dimensional size.
template<typename value_t, ENABLE_IF_ARITHMETIC(value_t)>
struct _Size2 {

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// @brief Width
    value_t width;

    /// @brief Height
    value_t height;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// @brief Default (non-initializing) constructor so this struct remains a POD
    _Size2() = default;

    /// @brief Value constructor.
    _Size2(const value_t width, const value_t height) : width(width), height(height) {}

    /// @brief Automatic type conversions between integer and real sizes.
    template<typename Other_t, ENABLE_IF_ARITHMETIC(Other_t)>
    _Size2(const _Size2<Other_t>& other)
        : width(static_cast<value_t>(other.width)), height(static_cast<value_t>(other.height))
    {
    }

    /// @brief Creates and returns an invalid Size2 instance.
    static _Size2 invalid() { return {-1, -1}; }

    /// @brief Creates and returns zero Size2 instance.
    static _Size2 zero() { return {0, 0}; }

    /// @brief The "most wrong" Size2 (maximal negative area).
    /// Is useful as the starting point for defining the union of multiple Size2.
    static _Size2 wrongest()
    {
        return {std::numeric_limits<value_t>::lowest(), std::numeric_limits<value_t>::lowest()};
    }

    /// @brief Tests if this Size is valid (>=0) in both dimensions.
    bool is_valid() const { return width >= 0 && height >= 0; }

    /// @brief Tests if a rectangle of this Size has zero area.
    bool is_zero() const { return abs(width) <= precision_high<value_t>() && abs(height) <= precision_high<value_t>(); }

    /// @brief Returns the area of a rectangle of this Size. Always returns 0, if the size is invalid.
    value_t area() const { return max(0, width * height); }

    /// @brief Pointer to the first element of the Size2.
    const value_t* as_ptr() const { return &width; }

    /// @brief Tests whether two Size2s are equal.
    bool operator==(const _Size2& other) const
    {
        return abs(other.width - width) <= precision_high<value_t>()
               && abs(other.height - height) <= precision_high<value_t>();
    }

    /// @brief Tests whether two not Size2s are equal.
    bool operator!=(const _Size2& other) const
    {
        return abs(other.width - width) > precision_high<value_t>()
               || abs(other.height - height) > precision_high<value_t>();
    }

    /// @brief Returns True, if other and self are approximately the same size.
    /// @param other Size to test against.*@param epsilon Maximal allowed distance between any of the two sides.
    bool is_approx(const _Size2& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return abs(other.width - width) <= epsilon && abs(other.height - height) <= epsilon;
    }

    /// @brief Changes the Size2 by a given factor.
    _Size2 operator*(const value_t factor) const { return {width * factor, height * factor}; }

    /// @brief Changes the Size2 by a given divisor.
    _Size2 operator/(const value_t divisor) const
    {
        if (divisor == 0) {
            throw std::domain_error("Division by zero");
        }
        return {width / divisor, height / divisor};
    }

    /// @brief Changes the Size2 by adding another.
    _Size2 operator+(const _Size2& other) const { return {width + other.width, height + other.width}; }

    /// @brief Adds another Size2 in-place.
    _Size2 operator+=(const _Size2& other)
    {
        width += other.width;
        height += other.height;
        return *this;
    }

    /// @brief Changes this Size2 to the maximum width and height of this and other.
    _Size2& maxed(const _Size2& other)
    {
        width  = max(width, other.width);
        height = max(height, other.height);
        return *this;
    }
};

//====================================================================================================================//

using Size2f = _Size2<float>;
using Size2i = _Size2<int>;
using Size2s = _Size2<short>;

// Free Functions ====================================================================================================//

/// @brief Prints the contents of this size into a std::ostream.
/// @param out   Output stream, implicitly passed with the << operator.
/// @param size  Size2 to print.
/// @return      Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Size2f& size);
std::ostream& operator<<(std::ostream& out, const Size2i& size);
std::ostream& operator<<(std::ostream& out, const Size2s& size);

} // namespace notf

// std::hash =========================================================================================================//

namespace std {

/// @brief std::hash specialization for notf::_RealVector2.
template<typename value_t>
struct hash<notf::_Size2<value_t>> {
    size_t operator()(const notf::_Size2<value_t>& size2) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::SIZE2), size2.width, size2.height);
    }
};

} // namespace std
