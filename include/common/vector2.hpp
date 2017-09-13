#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector2 : public detail::Arithmetic<_RealVector2<Real>, Real, 2> {

    /* Types **********************************************************************************************************/

    using super   = detail::Arithmetic<_RealVector2<Real>, Real, 2>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector2() = default;

    /** Element-wise constructor. */
    template <typename A, typename B>
    _RealVector2(A x, B y)
        : super{static_cast<value_t>(x), static_cast<value_t>(y)} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector2 along the X-axis. */
    static _RealVector2 x_axis() { return _RealVector2(1, 0); }

    /** Unit Vector2 along the Y-axis. */
    static _RealVector2 y_axis() { return _RealVector2(0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Returns True, if other and self are approximately the same Vector2.
     * @param other     Vector2 to test against.
     * @param epsilon   Maximal allowed distance between the two Vectors.
     */
    bool is_approx(const _RealVector2& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return (*this - other).get_magnitude_sq() <= epsilon * epsilon;
    }

    /** Checks whether this Vector2's direction is parallel to the other.
     * The zero Vector2 is parallel to every other Vector2.
     * @param other     Vector2 to test against.
     */
    bool is_parallel_to(const _RealVector2& other) const
    {
        if (abs(y()) <= precision_high<value_t>()) {
            return abs(other.y()) <= precision_high<value_t>();
        }
        else if (abs(other.y()) <= precision_high<value_t>()) {
            return abs(other.x()) <= precision_high<value_t>();
        }
        return abs((x() / y()) - (other.x() / other.y())) <= precision_low<value_t>();
    }

    /** Checks whether this Vector2 is orthogonal to the other.
     * The zero Vector2 is orthogonal to every other Vector2.
     * @param other     Vector2 to test against.
     */
    bool is_orthogonal_to(const _RealVector2& other) const
    {
        // normalization required for large absolute differences in vector lengths
        return abs(super::normalize().dot(other.normalize())) <= precision_high<value_t>();
    }

    /** Returns the smallest angle (in radians) to the other Vector2.
     * Always returns zero, if one or both of the input Vector2s are of zero magnitude.
     * @param other     Vector2 to test against.
     */
    value_t angle_to(const _RealVector2& other) const
    {
        const value_t mag_sq_product = super::get_magnitude_sq() * other.get_magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<value_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /** Tests if the other Vector2 is collinear (1), orthogonal(0), opposite (-1) or something in between.
     * Similar to `angle`, but saving a call to `acos`.
     * Returns zero, if one or both of the input Vector2s are of zero magnitude.
     * @param other     Vector2 to test against.
     */
    value_t direction_to(const _RealVector2& other) const
    {
        const value_t mag_sq_product = super::get_magnitude_sq() * other.get_magnitude_sq();
        if (mag_sq_product <= precision_high<value_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<value_t>()) {
            return clamp(dot(other), -1, 1); // both are unit
        }
        return clamp(dot(other) / sqrt(mag_sq_product), -1, 1);
    }

    /** Tests if this Vector2 is parallel to the X-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_horizontal() const { return abs(y()) <= precision_high<value_t>(); }

    /** Tests if this Vector2 is parallel to the Y-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_vertical() const { return abs(x()) <= precision_high<value_t>(); }

    /** Returns the slope of this Vector2.
     * If the Vector2 is parallel to the y-axis, the slope is infinite.
     */
    value_t slope() const
    {
        if (abs(x()) <= precision_high<value_t>()) {
            return INFINITY;
        }
        return y() / x();
    }

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Returns the dot product of this Vector2 and another.
     * @param other     Vector2 to the right.
     */
    value_t dot(const _RealVector2& other) const { return (x() * other.x()) + (y() * other.y()); }

    /** Returns the cross product of this Vector2 and another.
     * As defined at http://mathworld.wolfram.com/CrossProduct.html
     * @param other     Vector2 to the right.
     */
    value_t cross(const _RealVector2 other) const { return (x() * other.y()) - (y() * other.x()); }

    /** Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.
     * If the other Vector2 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector2 get_projected_on(const _RealVector2& other) { return other * dot(other); }

    /** Projects this Vector2 onto an infinite line whose direction is specified by other.
     * If the other Vector2 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector2 project_on(const _RealVector2& other)
    {
        *this = other * dot(other);
        return *this;
    }

    /** Returns a Vector2 orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
     * The resulting Vector2 is of the same magnitude as the original one.
     */
    _RealVector2 orthogonal() const { return _RealVector2(-y(), x()); }

    /** Rotates this Vector2 90 degree counter-clockwise. */
    _RealVector2& orthogonal()
    {
        const value_t temp = -y();
        y()                = x();
        x()                = temp;
        return *this;
    }

    /** Returns a copy of this 2D Vector, rotated counter-clockwise by a given angle (in radians). */
    _RealVector2 rotate(const value_t angle) const
    {
        const value_t sin_angle = sin(angle);
        const value_t cos_angle = cos(angle);
        return _RealVector2(
            (x() * cos_angle) - (y() * sin_angle),
            (y() * cos_angle) + (x() * sin_angle));
    }

    /** Rotates this 2D Vector counter-clockwise by a given angle (in radians). */
    _RealVector2& rotate(const value_t angle)
    {
        const value_t sin_angle = sin(angle);
        const value_t cos_angle = cos(angle);
        const value_t temp_x    = (x() * cos_angle) - (y() * sin_angle);
        const value_t temp_y    = (y() * cos_angle) + (x() * sin_angle);
        x()                     = temp_x;
        y()                     = temp_y;
        return *this;
    }

    /** Returns a copy this vector rotated around a pivot point by a given angle (in radians). */
    _RealVector2 get_rotated_around(const value_t angle, const _RealVector2& pivot) const
    {
        return pivot + (*this - pivot).rotate(angle);
    }

    /** Rotates this vector around a pivot point by a given angle (in radians). */
    _RealVector2 rotate_around(const value_t angle, const _RealVector2& pivot)
    {
        return *this = pivot + (pivot - *this).rotate(angle);
    }
};

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector2 containing integers. */
template <typename Integer, ENABLE_IF_INT(Integer)>
struct _IntVector2 : public detail::Arithmetic<_IntVector2<Integer>, Integer, 2> {

    // explitic forwards
    using super   = detail::Arithmetic<_IntVector2<Integer>, Integer, 2>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _IntVector2() = default;

    /** Perforect forwarding constructor. */
    template <typename... T>
    _IntVector2(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector2 along the X-axis. */
    static _IntVector2 x_axis() { return _IntVector2(1, 0); }

    /** Unit Vector2 along the Y-axis. */
    static _IntVector2 y_axis() { return _IntVector2(0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Tests if this Vector2 is parallel to the X-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_horizontal() const { return y() == 0; }

    /** Tests if this Vector2 is parallel to the Y-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_vertical() const { return x() == 0; }

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Returns a Vector2 orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
     * The resulting Vector2 is of the same magnitude as the original one.
     */
    _IntVector2 orthogonal() const { return _IntVector2(-y(), x()); }

    /** Rotates this Vector2 90 degree counter-clockwise. */
    _IntVector2& orthogonal()
    {
        const value_t temp = -y();
        y()                = x();
        x()                = temp;
        return *this;
    }
};

//*********************************************************************************************************************/

using Vector2f = _RealVector2<float>;
using Vector2d = _RealVector2<double>;
using Vector2i = _IntVector2<int>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of a Vector2 into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param vec  Vector2 to print.
 * @return Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Vector2f& vec);
std::ostream& operator<<(std::ostream& out, const Vector2d& vec);
std::ostream& operator<<(std::ostream& out, const Vector2i& vec);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_RealVector2. */
template <typename Real>
struct hash<notf::_RealVector2<Real>> {
    size_t operator()(const notf::_RealVector2<Real>& vector) const { return vector.hash(); }
};

/** std::hash specialization for notf::_IntVector2. */
template <typename Integer>
struct hash<notf::_IntVector2<Integer>> {
    size_t operator()(const notf::_IntVector2<Integer>& vector) const { return vector.hash(); }
};

} // namespace std
