#pragma once

#include "assert.h"
#include <array>
#include <iosfwd>

#include "common/float_utils.hpp"
#include "common/vector2.hpp"

namespace notf {

/// @brief A 2D Transformation Matrix with 3x3 components.
/// Only the first two columns are actually stored though, the last column is implicit.
/// [[a, b, 0]
///  [c, d, 0]
///  [e, f, 1]]
///
struct Transform2 {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Matrix values.
    mutable std::array<Vector2f, 3> rows;

    //  HOUSEHOLD  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Do not implement the default methods, so this data structure remains a POD.
    Transform2()                        = default;            // Default Constructor.
    ~Transform2()                       = default;            // Destructor.
    Transform2(const Transform2& other) = default;            // Copy Constructor.
    Transform2& operator=(const Transform2& other) = default; // Assignment Operator.

    //  STATIC CONSTRUCTORS  //////////////////////////////////////////////////////////////////////////////////////////

    /** The identity matrix. */
    static Transform2 identity()
    {
        return Transform2{{{{1, 0},
                            {0, 1},
                            {0, 0}}}};
    }

    /** A translation matrix. */
    static Transform2 translation(Vector2f translation)
    {
        return Transform2{{{{1, 0},
                            {0, 1},
                            std::move(translation)}}};
    }

    /// @brief A rotation matrix.
    /// @param radians  Counter-clockwise rotation in radians.
    static Transform2 rotation(const float radians)
    {
        const float sine   = sin(radians);
        const float cosine = cos(radians);
        return Transform2{{{{cosine, sine},
                            {-sine, cosine},
                            {0, 0}}}};
    }

    /// @brief A uniform scale matrix.
    /// @param factor   Uniform scale factor.
    static Transform2 scale(const float factor)
    {
        return Transform2{{{{factor, 0},
                            {0, factor},
                            {0, 0}}}};
    }

    /// @brief A non-uniform scale matrix.
    /// @param vector   Non-uniform scale vector.
    static Transform2 scale(const Vector2f& vec)
    {
        return Transform2{{{{vec.x, 0},
                            {0, vec.y},
                            {0, 0}}}};
    }

    /// @brief A non-uniform skew matrix.
    /// @param vector   Non-uniform skew vector.
    static Transform2 skew(const Vector2f& vec)
    {
        return Transform2{{{{1, tan(vec.y)},
                            {tan(vec.x), 1},
                            {0, 0}}}};
    }

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Returns the translation part of this Transform.
    Vector2f get_translation() const { return rows[2]; }

    /** Scale factor along the x-axis. */
    float get_scale_x() const { return sqrt(rows[0][0] * rows[0][0] + rows[1][0] * rows[1][0]); }

    /** Scale factor along the y-axis. */
    float get_scale_y() const { return sqrt(rows[0][1] * rows[0][1] + rows[1][1] * rows[1][1]); }

    /** Direct read-only access to the Matrix' memory. */
    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    const Vector2f& operator[](T&& row) const
    {
        assert(0 <= row && row <= 2);
        return rows[row];
    }

    /** Direct read-write access to the Matrix' memory. */
    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    Vector2f& operator[](T&& row)
    {
        assert(0 <= row && row <= 2);
        return rows[row];
    }

    /// @brief Equality operator.
    bool operator==(const Transform2& other) const { return rows == other.rows; }

    /// @brief Inequality operator.
    bool operator!=(const Transform2& other) const { return rows != other.rows; }

    /** Allows direct read-only access to the Transform2's internal storage. */
    const float* as_ptr() const { return &rows[0][0]; }

    /** Allows direct read/write access to the Transform2's internal storage. */
    float* as_ptr() { return &rows[0][0]; }

    //  MODIFICATION ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Matrix multiplication of this Matrix with another.
    Transform2 operator*(const Transform2& other) const
    {
        Transform2 result = *this;
        result *= other;
        return result;
    }

    /** Applies the other Transform to this one in-place. */
    Transform2& operator*=(const Transform2& other);

    /** Inverts this Transform2 in-place. */
    Transform2& invert(); // TODO: test Transform2::invert

    /** Returns the inverse of this Transform2. */
    Transform2 inverse() const;

    /** Returns a transformed Vector2. */
    Vector2f transform(const Vector2f& vector) const
    {
        return {
            vector.x * rows[0][0] + vector.y * rows[1][0] + rows[2][0],
            vector.x * rows[0][1] + vector.y * rows[1][1] + rows[2][1],
        };
    }
};

} // namespace notf
