#pragma once

#include <array>
#include <assert.h>
#include <iosfwd>

#include "common/vector2.hpp"

namespace notf {

namespace detail {

/** Transforms the given input and returns a new value. */
template <typename XFORM2, typename INPUT>
INPUT transform2(const XFORM2&, const INPUT&);

/** Transforms the given input in-place.
 * Modifies the argument but also returns a reference to it.
 */
template <typename XFORM2, typename INPUT>
INPUT& transform2(const XFORM2&, INPUT&);

} // namespace detail

//*********************************************************************************************************************/

/** A 2D transformation matrix with 3x3 components.
 * Only the first two rows are actually stored though, the last row is implicit.
 * [a, c, e,
 *  b, d, f
 *  0, 0, 1]
 */
template <typename REAL, bool SIMD_SPECIALIZATION = false, ENABLE_IF_REAL(REAL)>
struct _Xform2 : public detail::Arithmetic<_Xform2<REAL, SIMD_SPECIALIZATION, true>,
                                           _RealVector2<REAL>, 3, SIMD_SPECIALIZATION> {

    // explitic forwards
    using super    = detail::Arithmetic<_Xform2<REAL>, _RealVector2<REAL>, 3, SIMD_SPECIALIZATION>;
    using vector_t = _RealVector2<REAL>;
    using value_t  = REAL;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Xform2() = default;

    /** Value constructor defining the diagonal of the matrix. */
    explicit _Xform2(const value_t a)
        : super{vector_t{a, 0},
                vector_t{0, a},
                vector_t{0, 0}}
    {
    }

    /** Column-wise constructor of the matrix. */
    _Xform2(const vector_t a, const vector_t b, const vector_t c)
        : super{std::move(a),
                std::move(b),
                std::move(c)}
    {
    }

    /** Element-wise constructor. */
    _Xform2(const value_t a, const value_t b,
            const value_t c, const value_t d,
            const value_t e, const value_t f)
        : super{vector_t{a, b},
                vector_t{c, d},
                vector_t{e, f}}
    {
    }

    /* Static Constructors ********************************************************************************************/

    /** The identity matrix. */
    static _Xform2 identity() { return _Xform2(1); }

    /** A translation matrix. */
    static _Xform2 translation(vector_t translation)
    {
        return _Xform2(vector_t(1, 0),
                       vector_t(0, 1),
                       std::move(translation));
    }
    static _Xform2 translation(value_t x, value_t y) { return _Xform2::translation(vector_t(x, y)); }

    /** A rotation matrix.
     * @param radians   Counter-clockwise rotation in radians.
     */
    static _Xform2 rotation(const value_t radians)
    {
        const value_t sine   = sin(radians);
        const value_t cosine = cos(radians);
        return _Xform2(cosine, sine,
                       -sine, cosine,
                       0, 0);
    }

    /** A uniform scale matrix.
     * @param factor   Uniform scale factor.
     */
    static _Xform2 scaling(const value_t factor)
    {
        return _Xform2(factor, 0,
                       0, factor,
                       0, 0);
    }

    /** A non-uniform scale matrix.
     * You can also achieve reflection by passing (-1, 1) for a reflection over the vertical axis, (1, -1) for over the
     * horizontal axis or (-1, -1) for a point-reflection with respect to the origin.
     * @param vector   Non-uniform scale vector.
     */
    static _Xform2 scaling(const vector_t& vec)
    {
        return _Xform2(vec[0], 0,
                       0, vec[1],
                       0, 0);
    }
    static _Xform2 scaling(value_t x, value_t y) { return _Xform2::scaling(vector_t(x, y)); }

    /** A non-uniform skew matrix.
     * @param vector   Non-uniform skew vector.
     */
    static _Xform2 skew(const vector_t& vec)
    {
        return _Xform2(1, tan(vec[1]),
                       tan(vec[0]), 1,
                       0, 0);
    }
    static _Xform2 skew(value_t x, value_t y) { return _Xform2::skew(vector_t(x, y)); }

    /*  Inspection  ***************************************************************************************************/

    /** Returns the translation part of this Xform. */
    const vector_t& get_translation() const { return data[2]; }

    /** Returns the rotational part of this transformation.
     * Only works if this is actually a pure rotation matrix!
     * Use `is_rotation` to test, if in doubt.
     * @return  Applied rotation in radians.
     */
    value_t get_rotation() const
    {
        try {
            return atan2(data[0][1], data[1][1]);
        } catch (std::domain_error&) {
            return 0; // in case both values are zero
        }
    }

    /** Checks whether the matrix is a pure rotation matrix. */
    bool is_rotation() const
    {
        return (1 - get_determinant()) < precision_high<value_t>();
    }

    /** Scale factor along the x-axis. */
    value_t get_scale_x() const { return sqrt(data[0][0] * data[0][0] + data[0][1] * data[0][1]); }

    /** Scale factor along the y-axis. */
    value_t get_scale_y() const { return sqrt(data[1][0] * data[1][0] + data[1][1] * data[1][1]); }

    /** Calculates the determinant of the transformation matrix. */
    value_t get_determinant() const { return (data[0][0] * data[1][1]) - (data[1][0] * data[0][1]); }

    /** Modifiers *****************************************************************************************************/

    /** Concatenation of two transformation matrices. */
    _Xform2 operator*(const _Xform2& other) const
    {
        _Xform2 result;
        result[0][0] = data[0][0] * other[0][0] + data[1][0] * other[0][1],
        result[0][1] = data[0][1] * other[0][0] + data[1][1] * other[0][1],
        result[1][0] = data[0][0] * other[1][0] + data[1][0] * other[1][1],
        result[1][1] = data[0][1] * other[1][0] + data[1][1] * other[1][1],
        result[2][0] = data[0][0] * other[2][0] + data[1][0] * other[2][1] + data[2][0],
        result[2][1] = data[0][1] * other[2][0] + data[1][1] * other[2][1] + data[2][1];
        return result;
    }

    /** Applies another Xform to this one in-place. */
    _Xform2& operator*=(const _Xform2& other)
    {
        *this = *this * other;
        return *this;
    }

    /** Premultiplies the other transform matrix with this one. */
    _Xform2 premult(const _Xform2& other) const
    {
        return other * *this;
    }

    /** Premultiplies the other transform matrix with this one in-place. */
    _Xform2& premult(const _Xform2& other)
    {
        *this = other * *this;
        return *this;
    }

    /** Translates the transformation by a given delta vector. */
    _Xform2 translate(const vector_t& delta) const { return _Xform2(data[0], data[1], data[2] + delta); }

    /** Translates the transformation by a given delta vector in-place. */
    _Xform2& translate(const vector_t& delta)
    {
        data[2] += delta;
        return *this;
    }

    /** Rotates the transformation by a given angle in radians in-place. */
    _Xform2 rotate(const value_t radians) const { return _Xform2::rotation(radians) * *this; }

    /** Rotates the transformation by a given angle in radians in-place. */
    _Xform2& rotate(const value_t radians) { return premult(_Xform2::rotation(radians)); }

    /** Returns the inverse of this Xform2. */
    _Xform2 invert() const
    {
        const value_t det = get_determinant();
        if (abs(det) <= precision_high<value_t>()) {
            return _Xform2::identity();
        }
        const value_t invdet = 1 / det;

        _Xform2 result;
        result[0][0] = +(data[1][1]) * invdet;
        result[1][0] = -(data[1][0]) * invdet;
        result[2][0] = +(data[1][0] * data[2][1] - data[2][0] * data[1][1]) * invdet;
        result[0][1] = -(data[0][1]) * invdet;
        result[1][1] = +(data[0][0]) * invdet;
        result[2][1] = -(data[0][0] * data[2][1] - data[2][0] * data[0][1]) * invdet;
        return result;
    }

    /** Inverts this Xform2 in-place. */
    _Xform2& invert()
    {
        *this = const_cast<const _Xform2*>(this)->invert();
        return *this;
    }

    /** Transformation ************************************************************************************************/

    /** Transforms a given Vector and returns a new value. */
    vector_t transform(const vector_t& vector) const
    {
        return {
            data[0][0] * vector[0] + data[1][0] * vector[1] + data[2][0],
            data[0][1] * vector[0] + data[1][1] * vector[1] + data[2][1],
        };
    }

    /** Transforms a given Vector in-place.
     * Modifies the input vector but also returns a reference to it.
     */
    vector_t& transform(vector_t& vector) const
    {
        vector = {
            data[0][0] * vector[0] + data[1][0] * vector[1] + data[2][0],
            data[0][1] * vector[0] + data[1][1] * vector[1] + data[2][1],
        };
        return vector;
    }

    /** Transforms a given value and returns a new instance. */
    template <typename T>
    T transform(const T& value) const { return detail::transform2(*this, value); }

    /** Transforms a given value in-place.
     * Modifies the input but also returns a reference to it.
     */
    template <typename T>
    T& transform(T& value) const { return detail::transform2(*this, value); }
};

//*********************************************************************************************************************/

using Xform2f = _Xform2<float>;
using Xform2d = _Xform2<double>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Xform into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
template <typename REAL>
std::ostream& operator<<(std::ostream& out, const notf::_Xform2<REAL>& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_Aabr. */
template <typename Real>
struct hash<notf::_Xform2<Real>> {
    size_t operator()(const notf::_Xform2<Real>& xform2) const { return notf::hash(xform2[0], xform2[1], xform2[2]); }
};
}
