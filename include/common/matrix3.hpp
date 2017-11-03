#pragma once

#include "common/vector2.hpp"

namespace notf {

//====================================================================================================================//

namespace detail {

/// @brief Transforms the given input and returns a new value.
template<typename MATRIX3, typename T>
T matrix3_transform(const MATRIX3&, const T&);

} // namespace detail

//====================================================================================================================//

/// @brief A 2D transformation matrix with 3x3 components.
///
/// [a, c, e,
///  b, d, f
///  0, 0, 1]
///
/// Only the first two rows are actually stored though, the last row is implicit.
template<typename REAL>
struct _Matrix3 : public detail::Arithmetic<_Matrix3<REAL>, _RealVector2<REAL>, 3> {

    /// @brief Element type.
    using element_t = REAL;

    /// @brief Component type.
    using component_t = _RealVector2<element_t>;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_Matrix3<element_t>, component_t, 3>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _Matrix3() = default;

    /// @brief Value constructor defining the diagonal of the matrix.
    /// @param a    Value to put into the diagonal.
    _Matrix3(const element_t a) : super_t{component_t(a, 0), component_t(0, a), component_t(0, 0)} {}

    /// @brief Column-wise constructor of the matrix.
    /// @param a    First column.
    /// @param b    Second column.
    /// @param c    Third column.
    _Matrix3(const component_t a, const component_t b, const component_t c)
        : super_t{std::move(a), std::move(b), std::move(c)}
    {
    }

    /// @brief Element-wise constructor.
    _Matrix3(const element_t a, const element_t b, const element_t c, const element_t d, const element_t e,
             const element_t f)
        : super_t{component_t(a, b), component_t(c, d), component_t(e, f)}
    {
    }

    /// @brief The identity matrix.
    static _Matrix3 identity() { return _Matrix3(1); }

    /// @brief A translation matrix.
    /// @param translation  Translation vector.
    static _Matrix3 translation(const component_t translation)
    {
        return _Matrix3(component_t(1, 0), component_t(0, 1), std::move(translation));
    }

    /// @brief A translation matrix.
    /// @param x    X component of the translation vector.
    /// @param y    Y component of the translation vector.
    static _Matrix3 translation(const element_t x, const element_t y)
    {
        return _Matrix3::translation(component_t(x, y));
    }

    /// @brief A rotation matrix.
    /// @param radians   Counter-clockwise rotation in radians.
    static _Matrix3 rotation(const element_t radians)
    {
        const element_t sine   = sin(radians);
        const element_t cosine = cos(radians);
        return _Matrix3(cosine, sine, -sine, cosine, 0, 0);
    }

    /// @brief A uniform scale matrix.
    /// @param factor   Uniform scale factor.
    static _Matrix3 scaling(const element_t factor) { return _Matrix3(factor, 0, 0, factor, 0, 0); }

    /// @brief A non-uniform scale matrix.
    /// You can also achieve reflection by passing (-1, 1) for a reflection over the vertical axis, (1, -1) for over the
    /// horizontal axis or (-1, -1) for a point-reflection with respect to the origin.
    /// @param vec  Non-uniform scale vector.
    static _Matrix3 scaling(const component_t& vec) { return _Matrix3(vec[0], 0, 0, vec[1], 0, 0); }

    /// @brief A non-uniform scale matrix.
    /// @param x    X component of the scale vector.
    /// @param y    Y component of the scale vector.
    static _Matrix3 scaling(const element_t x, const element_t y) { return _Matrix3::scaling(component_t(x, y)); }

    /// @brief A non-uniform skew matrix.
    /// @param vec  Non-uniform skew vector.
    static _Matrix3 skew(const component_t& vec) { return _Matrix3(1, tan(vec[1]), tan(vec[0]), 1, 0, 0); }

    /// @brief A non-uniform skew matrix.
    /// @param x    X component of the skew vector.
    /// @param y    Y component of the skew vector.
    static _Matrix3 skew(const element_t x, const element_t y) { return _Matrix3::skew(component_t(x, y)); }

    /// @brief The translation part of this Xform.
    const component_t& translation() const { return data[2]; }

    /** Returns the rotational part of this transformation.
     * Only works if this is actually a pure rotation matrix!
     * Use `is_rotation` to test, if in doubt.
     * @return  Applied rotation in radians.
     */
    element_t rotation() const
    {
        try {
            return atan2(data[0][1], data[1][1]);
        }
        catch (std::domain_error&) {
            return 0; // in case both values are zero
        }
    }

    /// @brief Checks whether the matrix is a pure rotation matrix.
    bool is_rotation() const { return (1 - determinant()) < precision_high<element_t>(); }

    /// @brief Scale factor along the x-axis.
    element_t scale_x() const { return sqrt(data[0][0] * data[0][0] + data[0][1] * data[0][1]); }

    /// @brief Scale factor along the y-axis.
    element_t scale_y() const { return sqrt(data[1][0] * data[1][0] + data[1][1] * data[1][1]); }

    /// @brief Calculates the determinant of the transformation matrix.
    element_t determinant() const { return (data[0][0] * data[1][1]) - (data[1][0] * data[0][1]); }

    /// @brief Concatenation of two transformation matrices.
    /// @param other    Transformation to concatenate.
    _Matrix3 operator*(const _Matrix3& other) const
    {
        _Matrix3 result;
        result[0][0] = data[0][0] * other[0][0] + data[1][0] * other[0][1],
        result[0][1] = data[0][1] * other[0][0] + data[1][1] * other[0][1],
        result[1][0] = data[0][0] * other[1][0] + data[1][0] * other[1][1],
        result[1][1] = data[0][1] * other[1][0] + data[1][1] * other[1][1],
        result[2][0] = data[0][0] * other[2][0] + data[1][0] * other[2][1] + data[2][0],
        result[2][1] = data[0][1] * other[2][0] + data[1][1] * other[2][1] + data[2][1];
        return result;
    }

    /// @brief Concatenation of another transformation matrix to this one in-place.
    /// @param other    Transformation to concatenate.
    _Matrix3& operator*=(const _Matrix3& other)
    {
        *this = *this * other;
        return *this;
    }

    /// @brief Concatenate this to another another transformation matrix.
    /// @param other    Transformation to concatenate to.
    _Matrix3 premult(const _Matrix3& other) const { return other * *this; }

    /// @brief Translates this transformation by a given delta vector.
    /// @param delta    Delta translation.
    _Matrix3 translate(const component_t& delta) const { return _Matrix3(data[0], data[1], data[2] + delta); }

    /// @brief Rotates the transformation by a given angle in radians.
    /// @param radians  Rotation in radians.
    _Matrix3 rotate(const element_t radians) const { return _Matrix3::rotation(radians) * *this; }

    /// @brief Returns the inverse of this matrix.
    _Matrix3 inverse() const
    {
        const element_t det = determinant();
        if (abs(det) <= precision_high<element_t>()) {
            return _Matrix3::identity();
        }
        const element_t invdet = 1 / det;

        _Matrix3 result;
        result[0][0] = +(data[1][1]) * invdet;
        result[1][0] = -(data[1][0]) * invdet;
        result[2][0] = +(data[1][0] * data[2][1] - data[2][0] * data[1][1]) * invdet;
        result[0][1] = -(data[0][1]) * invdet;
        result[1][1] = +(data[0][0]) * invdet;
        result[2][1] = -(data[0][0] * data[2][1] - data[2][0] * data[0][1]) * invdet;
        return result;
    }

    /// @brief Returns the transformed copy of a given vector.
    /// @param vector   Vector to transform.
    component_t transform(const component_t& vector) const
    {
        return {
            data[0][0] * vector[0] + data[1][0] * vector[1] + data[2][0],
            data[0][1] * vector[0] + data[1][1] * vector[1] + data[2][1],
        };
    }

    /// @brief Return the transformed copy of the value.
    /// @param value    Value to transform.
    template<typename T>
    T transform(const T& value) const
    {
        return detail::matrix3_transform(*this, value);
    }
};

//====================================================================================================================//

using Matrix3f = _Matrix3<float>;
using Matrix3d = _Matrix3<double>;

//====================================================================================================================//

/// @brief Prints the contents of a matrix into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param matrix   Matrix to print.
/// @return Output stream for further output.
template<typename REAL>
std::ostream& operator<<(std::ostream& out, const notf::_Matrix3<REAL>& matrix);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for notf::_Matrix3.
template<typename Real>
struct hash<notf::_Matrix3<Real>> {
    size_t operator()(const notf::_Matrix3<Real>& xform2) const { return notf::hash(xform2[0], xform2[1], xform2[2]); }
};
} // namespace std
