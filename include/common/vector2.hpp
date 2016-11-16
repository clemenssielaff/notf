#pragma once

#include <iosfwd>

#include "common/float_utils.hpp"

namespace notf {

/// @brief The Vector2 class.
struct Vector2 {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief X coordinate.
    float x;

    /// @brief Y coordinate.
    float y;

    //  HOUSEHOLD  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Do not implement the default methods, so this data structure remains a POD.
    Vector2() = default; // Default Constructor.
    ~Vector2() = default; // Destructor.
    Vector2(const Vector2& other) = default; // Copy Constructor.
    Vector2& operator=(const Vector2& other) = default; // Assignment Operator.

    /// @brief Creates a Vector2 with the given components.
    ///
    /// @param x    X value.
    /// @param y    Y value.
    Vector2(float x, float y)
        : x(x)
        , y(y)
    {
    }

    //  STATIC CONSTRUCTORS  //////////////////////////////////////////////////////////////////////////////////////////

    /** Returns a zero Vector2. */
    static Vector2 zero() { return Vector2(0, 0); }

    /// @brief Returns a Vector2 with both components set to the given value.
    ///
    /// @param value    Value to set the components to.
    static Vector2 fill(float value) { return Vector2(value, value); }

    /// @brief Returns an unit Vector2 along the x-axis.
    static Vector2 x_axis() { return Vector2(1, 0); }

    /// @brief Returns an unit Vector2 along the y-axis.
    static Vector2 y_axis() { return Vector2(0, 1); }

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Checks, if this Vector2 is the zero vector.
    bool is_zero() const { return x == 0 && y == 0; }

    /// @brief Checks, if this Vector2 is approximately the zero vector.
    ///
    /// @param epsilon  A difference <= epsilon is considered equal.
    bool is_zero(float epsilon) const { return (approx(x, 0, epsilon) && approx(y, 0, epsilon)); }

    /// @brief Checks whether this Vector2 is of unit magnitude.
    bool is_unit() const { return approx(magnitude_sq(), 1); }

    /// @brief Checks whether this Vector2 is parallel to other.
    ///
    /// The zero-Vector is parallel to every Vector2.
    ///
    /// @param other    Vector2 to test against.
    bool is_parallel_to(const Vector2& other) const { return side_of(other) == 0; }

    /// @brief Checks whether this Vector2 is orthogonal to other.
    ///
    /// The zero-Vector is orthogonal to every Vector2.
    ///
    /// @param other    Vector2 to test against.
    bool is_orthogonal_to(const Vector2& other) const { return approx(dot(other), 0); }

    /// @brief The angle in radians between the positive x-axis and the point given by this Vector2.
    ///
    /// The angle is positive for counter-clockwise angles (upper half-plane, y > 0),
    /// and negative for clockwise angles (lower half-plane, y < 0).
    float angle() const { return atan2(y, x); }

    /// @brief Calculates the smallest angle between two Vector2s in radians.
    ///
    /// Returns zero, if one or both of the input Vector2%s are of zero magnitude.
    ///
    /// @param other    Vector2 to take the angle against.
    float angle_to(const Vector2& other) const
    {
        const float squaredMagnitudeProduct = magnitude_sq() * other.magnitude_sq();

        // one or both are zero
        if (approx(squaredMagnitudeProduct, 0)) {
            return 0;
        }

        // both are unit
        if (approx(squaredMagnitudeProduct, 1)) {
            return acos(dot(other));
        }

        // default
        return acos(dot(other) / sqrt(squaredMagnitudeProduct));
    }

    /// @brief Tests if the other Vector2 is collinear (1), orthogonal(0), opposite (-1) or something in between.
    ///
    /// Basically like getAngle(), but saving a call to 'acos'.
    /// Returns zero, if one or both of the input Vector2s are of zero magnitude.
    ///
    /// @param other   Vector2 to compare against.
    float direction_to(const Vector2& other) const
    {
        const float squaredMagnitudeProduct = magnitude_sq() * other.magnitude_sq();

        // one or both are zero
        if (approx(squaredMagnitudeProduct, 0)) {
            return 0;
        }

        // both are unit
        if (approx(squaredMagnitudeProduct, 1)) {
            return clamp(dot(other), -1, 1);
        }

        // default
        return clamp(dot(other) / sqrt(squaredMagnitudeProduct), -1, 1);
    }

    /// @brief Tests if this Vector2 is parallel to the x-axis.
    ///
    /// The zero vector is parallel to every Vector2.
    bool is_horizontal() const { return approx(y, 0); }

    /// @brief Tests if this Vector2 is parallel to the y-axis.
    ///
    /// The zero vector is parallel to every Vector2.
    bool is_vertical() const { return approx(x, 0); }

    /// @brief Returns True, if other and self are approximately the same Vector2.
    ///
    /// @param other    Vector2 to test against.
    bool is_approx(const Vector2& other) const { return (approx(x, other.x) && approx(y, other.y)); }

    /// @brief Returns True, if other and self are NOT approximately the same Vector2.
    ///
    /// @param other    Vector2 to test against.
    bool is_not_approx(const Vector2& other) const { return (!approx(x, other.x) || !approx(y, other.y)); }

    /// @brief Returns the slope of this Vector2.
    ///
    /// If the vector is parallel to the y-axis, the slope is infinite.
    float slope() const
    {
        if (x == 0.) {
            return INFINITY;
        }
        return y / x;
    }

    /// @brief Returns the squared magnitude of this Vector2.
    ///
    /// The squared magnitude is much cheaper to compute than the actual.
    float magnitude_sq() const { return dot(*this); }

    /// @brief Returns the magnitude of this Vector2.
    ///
    /// For comparisons of Vector2 magnitude or testing for normalization, use magnitude_sq instead.
    float magnitude() const { return sqrt((x * x) + (y * y)); }

    /// @brief Checks, if this Vector2 contains only float, finite values.
    ///
    /// INFINITY and NAN are not float numbers.
    bool is_real() const { return is_valid(x) && is_valid(y); }

    /// @brief Checks, if any component of this Vector2 is a zero.
    bool contains_zero() const { return approx(x, 0) || approx(y, 0); }

    //  OPERATORS  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Equal comparison with another Vector2.
    ///
    /// @param other    Other Vector2 to compare this one to.
    ///
    /// @return True, if the other Vector2 is equal to this one.
    bool operator==(const Vector2& other) const { return (other.x == x && other.y == y); }

    /// @brief Not-equal comparison with another Vector2.
    ///
    /// @param other    Other Vector2 to compare this one to.
    ///
    /// @return True, if the other Vector2 is not equal to this one.
    bool operator!=(const Vector2& other) const { return (other.x != x || other.y != y); }

    /// @brief Addition with another Vector2.
    ///
    /// @param other    Other Vector2 to add to this one.
    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }

    /// @brief In-place addition with another Vector2.
    ///
    /// @param other    Other Vector2 to add to this one.
    Vector2& operator+=(const Vector2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    /// @brief Subtraction of another Vector2.
    ///
    /// @param other    Other Vector2 to subtract from this one.
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }

    /// @brief In-place subtraction of another Vector2.
    ///
    /// @param other    Other Vector2 to subtract from this one.
    Vector2& operator-=(const Vector2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    /// @brief Component-wise multiplication of a Vector2 with another Vector2.
    ///
    /// @param other    Vector2 to multiply this Vector2 with.
    Vector2 operator*(const Vector2& other) const { return Vector2(x * other.x, y * other.y); }

    /// @brief In-place component-wise multiplication of a Vector2 with another Vector2.
    ///
    /// @param other    Vector2 to multiply this Vector2 with.
    Vector2& operator*=(const Vector2& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    /// @brief Component-wise division of a Vector2 by another Vector2.
    ///
    /// @param other   Factor to divide this Vector2 by.
    Vector2 operator/(const Vector2& other) const { return {x / other.x, y / other.y}; }

    /// @brief In-place component-wise division of a Vector2 by another Vector2.
    ///
    /// @param factor   Divisor to divide this Vector2 by.
    Vector2& operator/=(const Vector2& other)
    {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    /// @brief Multiplication of a Vector2 with a scalar value.
    ///
    /// @param factor   Factor to multiply this Vector2 with.
    Vector2 operator*(const float factor) const { return Vector2(x * factor, y * factor); }

    /// @brief In-place multiplication of a Vector2 with a scalar value.
    ///
    /// @param factor   Factor to multiply this Vector2 with.
    Vector2& operator*=(const float factor)
    {
        x *= factor;
        y *= factor;
        return *this;
    }

    /// @brief Division of a Vector2 by a scalar value.
    ///
    /// @param factor   Factor to divide this Vector2 by.
    Vector2 operator/(const float divisor) const { return Vector2(x / divisor, y / divisor); }

    /// @brief In-place division of a Vector2 by a scalar value.
    ///
    /// @param factor   Divisor to divide this Vector2 by.
    Vector2& operator/=(const float divisor)
    {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    /// @brief Unary negation operator -
    ///
    /// @return An inverted copy of this 2D Vector.
    Vector2 operator-() const { return inverted(); }

    //
    //  MODIFIERS  ////////////////////////////////////////////////////////////////////////////////////////////////////
    //

    /// @brief Sets all components of the Vector to zero.
    Vector2& set_null()
    {
        x = 0;
        y = 0;
        return *this;
    }

    /// @brief Returns an inverted copy of this Vector2.
    Vector2 inverted() const { return Vector2(-x, -y); }

    /// @brief Inverts this Vector2 in-place.
    Vector2& invert()
    {
        x = -x;
        y = -y;
        return *this;
    }

    /// @brief Vector2 dot product.
    ///
    /// Can be used to determine in which general direction a point lies in relation to another point.
    ///
    /// @param other    Vector2 to the right of the dot.
    float dot(const Vector2& other) const
    {
        return (x * other.x) + (y * other.y);
    }

    /// @brief Returns a normalized copy of this Vector2.
    Vector2 normalized() const
    {
        const float squaredMagnitude = magnitude_sq();

        // is unit
        if (approx(squaredMagnitude, 1)) {
            return Vector2(*this);
        }

        // is zero
        if (approx(squaredMagnitude, 0)) {
            return Vector2();
        }

        // default
        return (*this) / sqrt(squaredMagnitude);
    }

    /// @brief In-place normalization of this Vector2.
    Vector2& normalize()
    {
        const float squaredMagnitude = magnitude_sq();

        // is unit
        if (approx(squaredMagnitude, 1)) {
            return (*this);
        }

        // is zero
        if (approx(squaredMagnitude, 0)) {
            return set_null();
        }

        // default
        return (*this) /= sqrt(squaredMagnitude);
    }

    /// @brief Creates a projection of this Vector2 onto an infinite line whose direction is specified by other.
    ///
    /// If the other Vector2 is not normalized, the projection is scaled alongside with it.
    ///
    /// @param other    Vector2 defining the direction and scale of the projection canvas.
    ///
    /// @return A new Vector2 representing the projection.
    Vector2 projected_on(const Vector2& other) { return other * dot(other); }

    /// @brief Projects this Vector2 onto an infinite line whose direction is specified by other.
    ///
    /// If the other Vector2 is not normalized, the projection is scaled alongside with it.
    ///
    /// @param other    Vector2 defining the direction and scale of the projection canvas.
    ///
    /// @return This Vector2 projected on other.
    Vector2 project_on(const Vector2& other)
    {
        *this = other * dot(other);
        return *this;
    }

    /// @brief Creates an orthogonal 2D Vector to this one by rotating it 90 degree counter-clockwise.
    ///
    /// The resulting Vector2 is of the same magnitude as the original one.
    ///
    /// @return Vector2 orthogonal to this one.
    Vector2 orthogonal() const { return Vector2(-y, x); }

    /// @brief In-place rotation of this Vector2 90 degrees counter-clockwise.
    ///
    /// The resulting Vector2 is of the same magnitude as the original one.
    ///
    /// @return Vector2 orthogonal to this one.
    Vector2& orthogonalize()
    {
        const float temp = -y;
        y = x;
        x = temp;
        return *this;
    }

    /// @brief Returns a copy of this 2D Vector rotated around its origin by a given angle in radians.
    ///
    /// Rotation is applied counter-clockwise.
    ///
    /// @param angle    Angle to rotate in radians.
    ///
    /// @return A new, rotated Vector2.
    Vector2 rotated(const float angle) const
    {
        const float sinAngle = sin(angle);
        const float cosAngle = cos(angle);
        return Vector2(
            (x * cosAngle) - (y * sinAngle),
            (y * cosAngle) + (x * sinAngle));
    }

    /// @brief Rotates this Vector2 in-place around its origin by a given angle in radians.
    ///
    /// Rotation is applied counter-clockwise.
    ///
    /// @param angle    Angle to rotate in radians.
    ///
    /// @return This Vector2 rotated.
    Vector2& rotate(const float angle)
    {
        const float sinAngle = sin(angle);
        const float cosAngle = cos(angle);
        const float tempX = (x * cosAngle) - (y * sinAngle);
        const float tempY = (y * cosAngle) + (x * sinAngle);
        x = tempX;
        y = tempY;
        return (*this);
    }

    /// @brief Returns the side on which the other 2D Vector points to, as seen from the direction of this 2D Vector.
    ///
    /// I've decided to go with +1 to the left, because it is rotated a positive amount when defining "positive" as
    /// going counter-clockwise (which I do).
    ///
    /// @param other    2D Vector pointing to either side of this 2D Vector.
    ///
    /// @return +1 when other is on the left of this Vector, -1 when on the right
    ///         and 0 when it is straight ahead or behind.
    int side_of(const Vector2& other) const
    {
        // equivalent to: (other - (*this)).orthogonalize().dot(*this);
        const float direction = (other.x * y) - (x * other.y);

        // straight ahead or behind
        if (approx(direction, 0)) {
            return 0;
        }

        // on the right
        if (direction > 0) {
            return -1;
        }

        // on the left
        return 1;
    }
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Prints the contents of this Vector2 into a std::ostream.
///
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector2 to print.
///
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector2& vec);

/// @brief Constructs a duo of mutually orthogonal, normalized vectors.
///
/// (Exists mainly for symmetry with Vector3)
///
/// @param a    Reference vector, is normalized but not oriented.
/// @param b    Is normalized and oriented without regard to its former contents.
inline void orthonormal_basis(Vector2& a, Vector2& b)
{
    a.normalize();
    b = a.orthogonal();
}

/// @brief Linear interpolation between two Vector2%s.
///
/// @param from    Left Vector, full weight at bend = 0.
/// @param to      Right Vector, full weight at bend = 1.
/// @param blend   Blend value, clamped to range [0, 1].
///
/// @return Interpolated Vector2.
inline Vector2 lerp(const Vector2& from, const Vector2& to, const float blend)
{
    return from + ((to - from) * clamp(blend, 0, 1));
}

/// @brief Normalized linear interpolation between two Vector2%s.
///
/// @param from    Left Vector, active at fade <= 0.
/// @param to      Right Vector, active at fade >= 1.
/// @param blend   Blend value, clamped to [0 -> 1].
///
/// @return Interpolated Vector3.
inline Vector2 nlerp(const Vector2& from, const Vector2& to, const float blend)
{
    return lerp(from, to, blend).normalize();
}

} // namespace notf
