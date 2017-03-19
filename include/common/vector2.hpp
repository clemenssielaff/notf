#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector2 {

    /* Types **********************************************************************************************************/

    using Value_t = Real;

    /* Fields *********************************************************************************************************/

    /** X-coordinate. */
    Value_t x;

    /** Y-coordinate */
    Value_t y;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector2() = default;

    /** Creates a Vector2 with the given components.
     * @param x     X-coordinate.
     * @param y     Y-coordinate.
     */
    _RealVector2(const Value_t x, const Value_t y)
        : x(x), y(y) {}

    /* Static Constructors ********************************************************************************************/

    /** A zero Vector2. */
    static _RealVector2 zero() { return _RealVector2(0, 0); }

    /** Constructs a Vector2 with both coordinates set to the given value. */
    static _RealVector2 fill(const Value_t value) { return _RealVector2(value, value); }

    /** Unit Vector2 along the X-axis. */
    static _RealVector2 x_axis() { return _RealVector2(1, 0); }

    /** Unit Vector2 along the Y-axis. */
    static _RealVector2 y_axis() { return _RealVector2(0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Checks, if this Vector2 contains only real, finite values (no INFINITY or NAN). */
    bool is_real() const { return is_real(x) && is_real(y); }

    /** Returns true if both coordinates are (approximately) zero.
     * @param epsilon   Largest difference that is still considered to be zero.
     */
    bool is_zero(const Value_t epsilon = precision_high<Value_t>()) const { return abs(x) <= epsilon && abs(y) <= epsilon; }

    /** Checks, if any component of this Vector2 is a zero. */
    bool contains_zero() const { return abs(x) <= precision_high<Value_t>() || abs(y) <= precision_high<Value_t>(); }

    /** Checks whether this Vector2 is of unit magnitude. */
    bool is_unit() const { return abs(magnitude_sq() - 1) <= precision_high<Value_t>(); }

    /** Checks whether this Vector2's direction is parallel to the other.
     * The zero Vector2 is parallel to every other Vector2.
     * @param other     Vector2 to test against.
     */
    bool is_parallel_to(const _RealVector2& other) const
    {
        if (abs(y) <= precision_high<Value_t>()) {
            return abs(other.y) <= precision_high<Value_t>();
        }
        else if (abs(other.y) <= precision_high<Value_t>()) {
            return abs(other.x) <= precision_high<Value_t>();
        }
        return abs((x / y) - (other.x / other.y)) <= precision_low<Value_t>();
    }

    /** Checks whether this Vector2 is orthogonal to the other.
     * The zero Vector2 is orthogonal to every other Vector2.
     * @param other     Vector2 to test against.
     */
    bool is_orthogonal_to(const _RealVector2& other) const
    {
        // normalization required for large absolute differences in vector lengths
        return abs(normalized().dot(other.normalized())) <= precision_high<Value_t>();
    }

    /** Returns the angle (in radians) between the positive x-axis and this Vector2.
     * The angle is positive for counter-clockwise angles (upper half-plane, y > 0),
     * and negative for clockwise angles (lower half-plane, y < 0).
     */
    Value_t angle() const { return atan2(y, x); }

    /** Returns the smallest angle (in radians) to the other Vector2.
     * Always returns zero, if one or both of the input Vector2s are of zero magnitude.
     * @param other     Vector2 to test against.
     */
    Value_t angle_to(const _RealVector2& other) const
    {
        const Value_t mag_sq_product = magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<Value_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<Value_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /** Tests if the other Vector2 is collinear (1), orthogonal(0), opposite (-1) or something in between.
     * Similar to `angle`, but saving a call to `acos`.
     * Returns zero, if one or both of the input Vector2s are of zero magnitude.
     * @param other     Vector2 to test against.
     */
    Value_t direction_to(const _RealVector2& other) const
    {
        const Value_t mag_sq_product = magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<Value_t>()) {
            return 0; // one or both are zero
        }
        if (abs(mag_sq_product - 1) <= precision_high<Value_t>()) {
            return clamp(dot(other), -1, 1); // both are unit
        }
        return clamp(dot(other) / sqrt(mag_sq_product), -1, 1);
    }

    /** Tests if this Vector2 is parallel to the X-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_horizontal() const { return abs(y) <= precision_high<Value_t>(); }

    /** Tests if this Vector2 is parallel to the Y-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_vertical() const { return abs(x) <= precision_high<Value_t>(); }

    /** Returns True, if other and self are approximately the same Vector2.
     * @param other     Vector2 to test against.
     * @param epsilon   Maximal allowed distance between the two Vectors.
     */
    bool is_approx(const _RealVector2& other, const Value_t epsilon = precision_high<Value_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon * epsilon;
    }

    /** Returns the slope of this Vector2.
     * If the Vector2 is parallel to the y-axis, the slope is infinite.
     */
    Value_t slope() const
    {
        if (abs(x) <= precision_high<Value_t>()) {
            return INFINITY;
        }
        return y / x;
    }

    /** Returns the squared magnitude of this Vector2.
     * The squared magnitude is much cheaper to compute than the actual.
     */
    Value_t magnitude_sq() const { return dot(*this); }

    /** Returns the magnitude of this Vector2. */
    Value_t magnitude() const { return sqrt(dot(*this)); }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Vector2s are equal. */
    bool operator==(const _RealVector2& other) const { return (*this - other).is_zero(); }

    /** Tests whether two Vector2s not are equal. */
    bool operator!=(const _RealVector2& other) const { return !(*this == other); }

    /** Adding another Vector2 moves this Vector2 relative to its previous position. */
    _RealVector2 operator+(const _RealVector2& other) const { return _RealVector2(x + other.x, y + other.y); }

    /** In-place addition of another Vector2. */
    _RealVector2& operator+=(const _RealVector2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** Subtracting another Vector2 places this Vector2's start point at the other's end. */
    _RealVector2 operator-(const _RealVector2& other) const { return _RealVector2(x - other.x, y - other.y); }

    /** In-place subtraction of another Vector2. */
    _RealVector2& operator-=(const _RealVector2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /** Component-wise multiplication of a Vector2 with another Vector2. */
    _RealVector2 operator*(const _RealVector2& other) const { return _RealVector2(x * other.x, y * other.y); }

    /** In-place component-wise multiplication of a Vector2 with another Vector2. */
    _RealVector2& operator*=(const _RealVector2& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    /** Component-wise division of a Vector2 by another Vector2. */
    _RealVector2 operator/(const _RealVector2& other) const { return {save_div(x, other.x), save_div(y, other.y)}; }

    /** In-place component-wise division of a Vector2 by another Vector2. */
    _RealVector2& operator/=(const _RealVector2& other)
    {
        x = save_div(x, other.x);
        y = save_div(y, other.y);
        return *this;
    }

    /** Multiplication with a scalar scales this Vector2's length. */
    _RealVector2 operator*(const Value_t factor) const { return _RealVector2(x * factor, y * factor); }

    /** In-place multiplication with a scalar. */
    _RealVector2& operator*=(const Value_t factor)
    {
        x *= factor;
        y *= factor;
        return *this;
    }

    /** Division by a scalar inversely scales this Vector3's length.
     * If you know that divisor cannot be zero, calling `vector *= 1/divisor` will save you a 'division-by-zero' test.
     */
    _RealVector2 operator/(const Value_t divisor) const
    {
        if (divisor == 0) {
            throw division_by_zero();
        }
        return _RealVector2(x / divisor, y / divisor);
    }

    /** In-place division by a scalar value.
     * If you know that divisor cannot be zero, calling `vector *= 1/divisor` will save you a 'division-by-zero' test.
     */
    _RealVector2& operator/=(const Value_t divisor)
    {
        if (divisor == 0) {
            throw division_by_zero();
        }
        x /= divisor;
        y /= divisor;
        return *this;
    }

    /** Returns an inverted copy of this Vector2. */
    _RealVector2 operator-() const { return inverse(); }

    /** Read-only reference to an entry of the Vector2's internal storage. */
    template <typename Index, ENABLE_IF_INT(Index)>
    const Value_t& operator[](const Index index) const
    {
        if (0 > index || index > 1) {
            throw std::range_error("Index requested via Vector2 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Read-write reference to an entry of the Vector2's internal storage. */
    template <typename Index, ENABLE_IF_INT(Index)>
    Value_t& operator[](const Index index)
    {
        if (0 > index || index > 1) {
            throw std::range_error("Index requested via Vector2 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Read-only pointer to the Vector2's internal storage. */
    const Value_t* as_ptr() const { return &x; }

    /** Read-write pointer to the Vector2's internal storage. */
    Value_t* as_ptr() { return &x; }

    /** Modifiers *****************************************************************************************************/

    /** Sets all components to zero. */
    _RealVector2& set_zero()
    {
        x = y = 0;
        return *this;
    }

    /** Returns an inverted copy of this Vector2. */
    _RealVector2 inverse() const { return _RealVector2(-x, -y); }

    /** Inverts this Vector2 in-place. */
    _RealVector2& invert()
    {
        x = -x;
        y = -y;
        return *this;
    }

    /** Returns the dot product of this Vector2 and another.
     * @param other     Vector2 to the right.
     */
    Value_t dot(const _RealVector2& other) const { return (x * other.x) + (y * other.y); }

    /** Returns the cross product of this Vector2 and another.
     * As defined at http://mathworld.wolfram.com/CrossProduct.html
     * @param other     Vector2 to the right.
     */
    Value_t cross(const _RealVector2 other) const
    {
        return (x * other.y) - (y * other.x);
    }

    /** Returns a normalized copy of this Vector2. */
    _RealVector2 normalized() const
    {
        const Value_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<Value_t>()) {
            return _RealVector2(*this); // is unit
        }
        if (abs(mag_sq) <= precision_high<Value_t>()) {
            return _RealVector2(); // is zero
        }
        return (*this) * (1 / sqrt(mag_sq));
    }

    /** In-place normalization of this Vector2. */
    _RealVector2& normalize()
    {
        const Value_t mag_sq = magnitude_sq();
        if (abs(mag_sq - 1) <= precision_high<Value_t>()) {
            return (*this); // is unit
        }
        if (abs(mag_sq) <= precision_high<Value_t>()) {
            return set_zero(); // is zero
        }
        return (*this) *= (1 / sqrt(mag_sq));
    }

    /** Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.
     * If the other Vector2 is not normalized, the projection is scaled alongside with it.
     */
    _RealVector2 projected_on(const _RealVector2& other) { return other * dot(other); }

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
    _RealVector2 orthogonal() const { return _RealVector2(-y, x); }

    /** Rotates this Vector2 90 degree counter-clockwise. */
    _RealVector2& orthogonalize()
    {
        const Value_t temp = -y;
        y                  = x;
        x                  = temp;
        return *this;
    }

    /** Returns a copy of this 2D Vector, rotated counter-clockwise by a given angle (in radians). */
    _RealVector2 rotated(const Value_t angle) const
    {
        const Value_t sin_angle = sin(angle);
        const Value_t cos_angle = cos(angle);
        return _RealVector2(
            (x * cos_angle) - (y * sin_angle),
            (y * cos_angle) + (x * sin_angle));
    }

    /** Rotates this 2D Vector counter-clockwise by a given angle (in radians). */
    _RealVector2& rotate(const Value_t angle)
    {
        const Value_t sin_angle = sin(angle);
        const Value_t cos_angle = cos(angle);
        const Value_t temp_x    = (x * cos_angle) - (y * sin_angle);
        const Value_t temp_y    = (y * cos_angle) + (x * sin_angle);
        x                       = temp_x;
        y                       = temp_y;
        return (*this);
    }

    /** Returns the side on which the other Vector2 points to, relative to the direction of this Vector2.
     * @return +1 when other is on the left, -1 when on the right and 0 when it is straight ahead or behind.
     */
    Value_t side_of(const _RealVector2& other) const
    {
        const Value_t direction = cross(other);
        if (abs(direction) <= precision_high<Value_t>()) {
            return 0;
        }
        return sign(direction);
    }
};

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector2 containing integers. */
template <typename Integer, ENABLE_IF_INT(Integer)>
struct _IntVector2 {

    /** X-coordinate. */
    Integer x;

    /** Y-coordinate */
    Integer y;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _IntVector2() = default;

    /** Creates a Vector2 with the given components.
     * @param x     X-coordinate.
     * @param y     Y-coordinate.
     */
    _IntVector2(Integer x, Integer y)
        : x(x), y(y) {}

    /* Static Constructors ********************************************************************************************/

    /** A zero Vector2. */
    static _IntVector2 zero() { return _IntVector2(0, 0); }

    /** Constructs a Vector2 with both coordinates set to the given value. */
    static _IntVector2 fill(Integer value) { return _IntVector2(value, value); }

    /** Unit Vector2 along the X-axis. */
    static _IntVector2 x_axis() { return _IntVector2(1, 0); }

    /** Unit Vector2 along the Y-axis. */
    static _IntVector2 y_axis() { return _IntVector2(0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Returns true if both coordinates are zero.  */
    bool is_zero() const { return x == 0 && y == 0; }

    /** Tests if this Vector2 is parallel to the X-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_horizontal() const { return y == 0; }

    /** Tests if this Vector2 is parallel to the Y-axis.
     * The zero Vector2 is parallel to every Vector2.
     */
    bool is_vertical() const { return x == 0; }

    /** Checks, if any component of this Vector2 is a zero. */
    bool contains_zero() const { return x == 0 || y == 0; }

    /** Direct read-only memory access to the Vector2. */
    template <typename Index, ENABLE_IF_INT(Index)>
    const Integer& operator[](const Index index) const
    {
        if (0 > index || index > 1) {
            throw std::range_error("Index requested via Vector2 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Direct read/write memory access to the Vector2. */
    template <typename Index, ENABLE_IF_INT(Index)>
    Integer& operator[](const Index index)
    {
        if (0 > index || index > 1) {
            throw std::range_error("Index requested via Vector2 [] operator is out of bounds");
        }
        return *(&x + index);
    }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Vector2s are equal. */
    bool operator==(const _IntVector2& other) const { return other.x == x && other.y == y; }

    /** Tests whether two Vector2s not are equal. */
    bool operator!=(const _IntVector2& other) const { return other.x != x || other.y != y; }

    /** Adding another Vector2 moves this Vector2 relative to its previous position. */
    _IntVector2 operator+(const _IntVector2& other) const { return _IntVector2(x + other.x, y + other.y); }

    /** In-place addition of another Vector2. */
    _IntVector2& operator+=(const _IntVector2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** Subtracting another Vector2 places this Vector2's start point at the other's end. */
    _IntVector2 operator-(const _IntVector2& other) const { return _IntVector2(x - other.x, y - other.y); }

    /** In-place subtraction of another Vector2. */
    _IntVector2& operator-=(const _IntVector2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /** Component-wise multiplication of a Vector2 with another Vector2. */
    _IntVector2 operator*(const _IntVector2& other) const { return _IntVector2(x * other.x, y * other.y); }

    /** In-place component-wise multiplication of a Vector2 with another Vector2. */
    _IntVector2& operator*=(const _IntVector2& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    /** Multiplication with a scalar scales this Vector2's length. */
    _IntVector2 operator*(const Integer factor) const { return _IntVector2(x * factor, y * factor); }

    /** In-place multiplication with a scalar. */
    _IntVector2& operator*=(const Integer factor)
    {
        x *= factor;
        y *= factor;
        return *this;
    }

    /** Returns an inverted copy of this Vector2. */
    _IntVector2 operator-() const { return inverse(); }

    /** Modifiers *****************************************************************************************************/

    /** Sets all components to zero. */
    _IntVector2& set_zero()
    {
        x = 0;
        y = 0;
        return *this;
    }

    /** Returns an inverted copy of this Vector2. */
    _IntVector2 inverse() const { return _IntVector2(-x, -y); }

    /** Inverts this Vector2 in-place. */
    _IntVector2& invert()
    {
        x = -x;
        y = -y;
        return *this;
    }

    /** Returns a Vector2 orthogonal to this one, by rotating the copy 90 degree counter-clockwise.
     * The resulting Vector2 is of the same magnitude as the original one.
     */
    _IntVector2 orthogonal() const { return _IntVector2(-y, x); }

    /** Rotates this Vector2 90 degree counter-clockwise. */
    _IntVector2& orthogonalize()
    {
        const Integer temp = -y;
        y                  = x;
        x                  = temp;
        return *this;
    }
};

//*********************************************************************************************************************/

using Vector2f = _RealVector2<float>;
using Vector2d = _RealVector2<double>;
using Vector2i = _IntVector2<int>;

/* Free Functions *****************************************************************************************************/

/** Linear interpolation between two Vector2s.
 * @param from    Left Vector, full weight at blend <= 0.
 * @param to      Right Vector, full weight at blend >= 1.
 * @param blend   Blend value, clamped to range [0, 1].
 */
template <typename Real>
inline _RealVector2<Real> lerp(const _RealVector2<Real>& from, const _RealVector2<Real>& to, const Real blend)
{
    return ((to - from) *= clamp(blend, 0, 1)) += from;
}

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
    size_t operator()(const notf::_RealVector2<Real>& vector) const { return notf::hash(vector.x, vector.y); }
};

/** std::hash specialization for notf::_IntVector2. */
template <typename Integer>
struct hash<notf::_IntVector2<Integer>> {
    size_t operator()(const notf::_IntVector2<Integer>& vector) const { return notf::hash(vector.x, vector.y); }
};

} // namespace std
