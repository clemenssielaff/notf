#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/**
 * Two-dimensional size.
 */
template <typename Value_t, ENABLE_IF_ARITHMETIC(Value_t)>
struct _Size2 {

    /* Fields *********************************************************************************************************/

    /** Width */
    Value_t width;

    /** Height */
    Value_t height;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Size2() = default;

    /** Value constructor. */
    _Size2(const Value_t width, const Value_t height)
        : width(width)
        , height(height)
    {
    }

    /** Automatic type conversions between integer and real sizes. */
    template <typename Other_t, ENABLE_IF_ARITHMETIC(Other_t)>
    _Size2(const _Size2<Other_t>& other)
        : width(static_cast<Value_t>(other.width))
        , height(static_cast<Value_t>(other.height))
    {
    }

    /* Static Constructors ********************************************************************************************/

    /** Creates and returns an invalid Size2 instance. */
    static _Size2 invalid() { return {-1, -1}; }

    /** Creates and returns zero Size2 instance. */
    static _Size2 zero() { return {0, 0}; }

    /* Inspection  ****************************************************************************************************/

    /** Tests if this Size is valid (>=0) in both dimensions. */
    bool is_valid() const { return width >= 0 && height >= 0; }

    /** Tests if a rectangle of this Size had zero area. */
    bool is_zero() const // TODO: we have both is_null and is_zero
    {
        return abs(width) <= precision_high<Value_t>()
            && abs(height) <= precision_high<Value_t>();
    }

    /** Returns the area of a rectangle of this Size. Always returns 0, if the size is invalid. */
    Value_t area() const { return max(0, width * height); }

    /** Pointer to the first element of the Size2. */
    const Value_t* as_ptr() const { return &width; }

    /* Operators ******************************************************************************************************/

    /** Tests whether two Size2s are equal. */
    bool operator==(const _Size2& other) const
    {
        return abs(other.width - width) <= precision_high<Value_t>()
            && abs(other.height - height) <= precision_high<Value_t>();
    }

    /** Tests whether two not Size2s are equal. */
    bool operator!=(const _Size2& other) const
    {
        return abs(other.width - width) > precision_high<Value_t>()
            || abs(other.height - height) > precision_high<Value_t>();
    }

    /** Changes the Size2 by a given factor. */
    _Size2 operator*(const Value_t factor) const { return {width * factor, height * factor}; }
};

//*********************************************************************************************************************/

using Size2f = _Size2<float>;
using Size2i = _Size2<int>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Size2f into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param size  Size2f to print.
 * @return      Output stream for further output.
 */
template <typename Value_t>
std::ostream& operator<<(std::ostream& out, const _Size2<Value_t>& size);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_RealVector2. */
template <typename Value_t>
struct hash<notf::_Size2<Value_t>> {
    size_t operator()(const notf::_Size2<Value_t>& size2) const { return notf::hash(size2.width, size2.height); }
};

} // namespace std
