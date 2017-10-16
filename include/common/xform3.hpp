#pragma once

#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "common/xform2.hpp"

namespace notf {

namespace detail {

/** Transforms the given input and returns a new value. */
template <typename XFORM3, typename INPUT>
INPUT transform3(const XFORM3&, const INPUT&);

/** Transforms the given input in-place.
 * Modifies the argument but also returns a reference to it.
 */
template <typename XFORM3, typename INPUT>
INPUT& transform3(const XFORM3&, INPUT&);

} // namespace detail

//*********************************************************************************************************************/

/** A full 3D Transformation matrix with 4x4 components.
 * [a, e, i, m
 *  b, f, j, n
 *  c, g, k, o
 *  d, h, l, p]
 * Matrix layout is equivalent to glm's layout, which in turn is equivalent to GLSL's matrix layout for easy
 * compatiblity with OpenGL.
 */
template <typename Real, bool SIMD_SPECIALIZATION = false, ENABLE_IF_REAL(Real)>
struct _Xform3 : public detail::Arithmetic<_Xform3<Real, SIMD_SPECIALIZATION, true>, _RealVector4<Real>, 4, SIMD_SPECIALIZATION> {

    // TODO: this whole SIMD_SPECIALIZATION means that you cannot propery forward define _Xform3 anymore ... :/

    // explitic forwards
    using super    = detail::Arithmetic<_Xform3<Real>, _RealVector4<Real>, 4, SIMD_SPECIALIZATION>;
    using vector_t = _RealVector4<Real>;
    using value_t  = Real;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Xform3() = default;

    /** Value constructor defining the diagonal of the matrix. */
    explicit _Xform3(const value_t a)
        : super{vector_t(a, 0, 0, 0),
                vector_t(0, a, 0, 0),
                vector_t(0, 0, a, 0),
                vector_t(0, 0, 0, a)}
    {
    }

    /** Column-wise constructor of the matrix. */
    _Xform3(const vector_t a, const vector_t b, const vector_t c, const vector_t d)
        : super{std::move(a),
                std::move(b),
                std::move(c),
                std::move(d)}
    {
    }

    /** Element-wise constructor. */
    _Xform3(const value_t a, const value_t b, const value_t c, const value_t d,
            const value_t e, const value_t f, const value_t g, const value_t h,
            const value_t i, const value_t j, const value_t k, const value_t l,
            const value_t m, const value_t n, const value_t o, const value_t p)
        : super{vector_t{a, b, c, d},
                vector_t{e, f, g, h},
                vector_t{i, j, k, l},
                vector_t{m, n, o, p}}
    {
    }

    /* Static Constructors ********************************************************************************************/

    /** Identity Transform */
    static _Xform3 identity() { return _Xform3(1); }

    /** A 2D translation matrix. */
    static _Xform3 translation(const Vector2f& t)
    {
        return _Xform3(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       t.x(), t.y(), 0, 1);
    }

    /** A 3D translation matrix. */
    static _Xform3 translation(const Vector4f& t)
    {
        return _Xform3(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       t.x(), t.y(), t.z(), 1);
    }
    static _Xform3 translation(const value_t x, const value_t y, const value_t z = 0)
    {
        return _Xform3(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       x, y, z, 1);
    }

    /** A rotation matrix. */
    static _Xform3 rotation(const vector_t axis, const value_t radians)
    {
        _Xform3 result = identity();
        result.rotate(std::move(axis), radians);
        return result;
    }

    /** A uniform scale matrix.
     * @param factor   Uniform scale factor.
     */
    static _Xform3 scaling(const value_t scale)
    {
        return _Xform3(scale, 0, 0, 0,
                       0, scale, 0, 0,
                       0, 0, scale, 0,
                       0, 0, 0, 1);
    }
    /** A non-uniform scale matrix.
     * @param vector   Non-uniform scale vector.
     */
    static _Xform3 scaling(const vector_t& scale)
    {
        return _Xform3(scale[0], 0, 0, 0,
                       0, scale[1], 0, 0,
                       0, 0, scale[2], 0,
                       0, 0, 0, 1);
    }

    /** Creates a perspective transformation.
     * @param fov       Horizontal field of view in radians.
     * @param aspect    Aspect ratio (width / height)
     * @param znear     Distance to the near plane in z direction.
     * @param zfar      Distance to the far plane in z direction.
     */
    static _Xform3 perspective(const value_t fov, const value_t aspect, value_t near, value_t far)
    {
        // near and far planes must be >= 1
        near = max(near, 1);
        far  = max(near, far);

        _Xform3 result = _Xform3::zero();
        if (std::abs(aspect) <= precision_high<value_t>()
            || std::abs(far - near) <= precision_high<value_t>()) {
            return result;
        }

        const value_t tan_half_fov = tan(fov / 2);

        result[0][0] = 1 / (aspect * tan_half_fov);
        result[1][1] = 1 / tan_half_fov;
        result[2][3] = -1;
        result[2][2] = -(far + near) / (far - near);
        result[3][2] = -(2 * far * near) / (far - near);

        return result;
    }

    /** Creates an orthographic transformation matrix.
     */
    static _Xform3 orthographic(const value_t left, const value_t right, const value_t bottom, const value_t top,
                                value_t near, value_t far)
    {
        // near and far planes must be >= 1
        near = max(near, 1);
        far  = max(near, far);

        const value_t width  = right - left;
        const value_t height = top - bottom;
        const value_t depth  = far - near;

        _Xform3 result = identity();
        if (std::abs(width) <= precision_high<value_t>()
            || std::abs(height) <= precision_high<value_t>()
            || std::abs(depth) <= precision_high<value_t>()) {
            return result;
        }

        result[0][0] = 2 / width;
        result[1][1] = 2 / height;
        result[3][0] = -(right + left) / width;
        result[3][1] = -(top + bottom) / height;
        result[2][2] = -2 / depth;
        result[3][2] = -(near + far) / depth;

        return result;
    }

    /* Inspection *****************************************************************************************************/

    /** Returns the translation component of this Xform. */
    const vector_t& get_translation() const { return data[3]; }

    /* Modification ***************************************************************************************************/

    /** Concatenation of two transformation matrices. */
    _Xform3 operator*(const _Xform3& other) const
    {
        _Xform3 result;
        result[0] = data[0] * other[0][0] + data[1] * other[0][1] + data[2] * other[0][2] + data[3] * other[0][3];
        result[1] = data[0] * other[1][0] + data[1] * other[1][1] + data[2] * other[1][2] + data[3] * other[1][3];
        result[2] = data[0] * other[2][0] + data[1] * other[2][1] + data[2] * other[2][2] + data[3] * other[2][3];
        result[3] = data[0] * other[3][0] + data[1] * other[3][1] + data[2] * other[3][2] + data[3] * other[3][3];
        return result;
    }

    /** Applies the other Xform to this one in-place. */
    _Xform3& operator*=(const _Xform3& other)
    {
        *this = *this * other;
        return *this;
    }

    /** Premultiplies the other transform matrix with this one in-place. */
    _Xform3& premult(const _Xform3& other)
    {
        *this = other * *this;
        return *this;
    }

    /** Applies a right-hand rotation around the given axis to this xform. */
    _Xform3& translate(const vector_t& delta)
    {
        data[3] = data[0] * delta[0] + data[1] * delta[1] + data[2] * delta[2] + data[3];
        return *this;
    }

    /** Applies a right-hand rotation around the given axis to this xform. */
    _Xform3& rotate(vector_t axis, const value_t radian)
    {
        const value_t cos_angle = cos(radian);
        const value_t sin_angle = sin(radian);

        axis[3] = 0;
        axis.normalize(); // TODO: what to do about V4 normalization?
        axis[3] = 1;
        const vector_t temp = axis * (1 - cos_angle);

        _Xform3 rotation;
        rotation[0][0] = cos_angle + temp[0] * axis[0];
        rotation[0][1] = temp[0] * axis[1] + sin_angle * axis[2];
        rotation[0][2] = temp[0] * axis[2] - sin_angle * axis[1];

        rotation[1][0] = temp[1] * axis[0] - sin_angle * axis[2];
        rotation[1][1] = cos_angle + temp[1] * axis[1];
        rotation[1][2] = temp[1] * axis[2] + sin_angle * axis[0];

        rotation[2][0] = temp[2] * axis[0] + sin_angle * axis[1];
        rotation[2][1] = temp[2] * axis[1] - sin_angle * axis[0];
        rotation[2][2] = cos_angle + temp[2] * axis[2];

        _Xform3 result;
        result[0] = data[0] * rotation[0][0] + data[1] * rotation[0][1] + data[2] * rotation[0][2];
        result[1] = data[0] * rotation[1][0] + data[1] * rotation[1][1] + data[2] * rotation[1][2];
        result[2] = data[0] * rotation[2][0] + data[1] * rotation[2][1] + data[2] * rotation[2][2];
        result[3] = data[3];

        *this = result;
        return *this;
    }

    /** Applies a non-uniform scaling to this xform. */
    _Xform3& scale(const vector_t& factor) const
    {
        data[0] *= factor[0];
        data[1] *= factor[1];
        data[2] *= factor[2];
        return *this;
    }

    /** Applies a non-uniform scaling to this xform. */
    _Xform3& scale(const value_t& factor) const
    {
        data[0] *= factor;
        data[1] *= factor;
        data[2] *= factor;
        return *this;
    }

    /** Returns the inverse of this Xform2. */
    _Xform3 get_inverse() const
    {
        value_t coef00 = data[2][2] * data[3][3] - data[3][2] * data[2][3];
        value_t coef02 = data[1][2] * data[3][3] - data[3][2] * data[1][3];
        value_t coef03 = data[1][2] * data[2][3] - data[2][2] * data[1][3];

        value_t coef04 = data[2][1] * data[3][3] - data[3][1] * data[2][3];
        value_t coef06 = data[1][1] * data[3][3] - data[3][1] * data[1][3];
        value_t coef07 = data[1][1] * data[2][3] - data[2][1] * data[1][3];

        value_t coef08 = data[2][1] * data[3][2] - data[3][1] * data[2][2];
        value_t coef10 = data[1][1] * data[3][2] - data[3][1] * data[1][2];
        value_t coef11 = data[1][1] * data[2][2] - data[2][1] * data[1][2];

        value_t coef12 = data[2][0] * data[3][3] - data[3][0] * data[2][3];
        value_t coef14 = data[1][0] * data[3][3] - data[3][0] * data[1][3];
        value_t coef15 = data[1][0] * data[2][3] - data[2][0] * data[1][3];

        value_t coef16 = data[2][0] * data[3][2] - data[3][0] * data[2][2];
        value_t coef18 = data[1][0] * data[3][2] - data[3][0] * data[1][2];
        value_t coef19 = data[1][0] * data[2][2] - data[2][0] * data[1][2];

        value_t coef20 = data[2][0] * data[3][1] - data[3][0] * data[2][1];
        value_t coef22 = data[1][0] * data[3][1] - data[3][0] * data[1][1];
        value_t coef23 = data[1][0] * data[2][1] - data[2][0] * data[1][1];

        vector_t fac0(coef00, coef00, coef02, coef03);
        vector_t fac1(coef04, coef04, coef06, coef07);
        vector_t fac2(coef08, coef08, coef10, coef11);
        vector_t fac3(coef12, coef12, coef14, coef15);
        vector_t fac4(coef16, coef16, coef18, coef19);
        vector_t fac5(coef20, coef20, coef22, coef23);

        vector_t vec0(data[1][0], data[0][0], data[0][0], data[0][0]);
        vector_t vec1(data[1][1], data[0][1], data[0][1], data[0][1]);
        vector_t vec2(data[1][2], data[0][2], data[0][2], data[0][2]);
        vector_t vec3(data[1][3], data[0][3], data[0][3], data[0][3]);

        vector_t inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
        vector_t inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
        vector_t inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
        vector_t inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

        vector_t signA(+1, -1, +1, -1);
        vector_t signB(-1, +1, -1, +1);
        _Xform3 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

        vector_t row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);
        vector_t dot0(data[0] * row0);
        value_t dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

        return inverse / dot1;
    }

    /** Inverts this Xform in-place. */
    _Xform3& invert()
    {
        *this = get_inverse();
        return *this;
    }

    /* Transformation *************************************************************************************************/

    /** Transforms a given Vector in-place.
     * Modifies the input vector but also returns a reference to it.
     */
    _RealVector4<value_t>& transform(_RealVector4<value_t>& vector) const
    {
        // this is the operation matrix * vector
        const _RealVector4<value_t> mov0 = _RealVector4<value_t>::fill(vector[0]);
        const _RealVector4<value_t> mov1 = _RealVector4<value_t>::fill(vector[1]);
        const _RealVector4<value_t> mul0 = data[0] * mov0;
        const _RealVector4<value_t> mul1 = data[1] * mov1;
        const _RealVector4<value_t> mov2 = _RealVector4<value_t>::fill(vector[2]);
        const _RealVector4<value_t> mov3 = _RealVector4<value_t>::fill(vector[3]);
        const _RealVector4<value_t> mul2 = data[2] * mov2;
        const _RealVector4<value_t> mul3 = data[3] * mov3;

        const _RealVector4<value_t> add0 = mul0 + mul1;
        const _RealVector4<value_t> add1 = mul2 + mul3;

        vector = add0 + add1;

        // this is the operation vector * matrix
        //        vector = vector_t(
        //            data[0][0] * vector[0] + data[0][1] * vector[1] + data[0][2] * vector[2] + data[0][3] * vector[3],
        //            data[1][0] * vector[0] + data[1][1] * vector[1] + data[1][2] * vector[2] + data[1][3] * vector[3],
        //            data[2][0] * vector[0] + data[2][1] * vector[1] + data[2][2] * vector[2] + data[2][3] * vector[3],
        //            data[3][0] * vector[0] + data[3][1] * vector[1] + data[3][2] * vector[2] + data[3][3] * vector[3]);
        return vector;
    }

    /** Transforms a given Vector and returns a new value. */
    _RealVector4<value_t> transform(const _RealVector4<value_t>& vector) const
    {
        _RealVector4<value_t> result = vector;
        transform(result);
        return result;
    }

    /** Transforms a given Vector in-place.
     * Modifies the input vector but also returns a reference to it.
     */
    _RealVector2<value_t>& transform(_RealVector2<value_t>& vector) const
    {
        vector = transform(const_cast<const _RealVector2<value_t>&>(vector));
        return vector;
    }

    /** Transforms a given Vector and returns a new value. */
    _RealVector2<value_t> transform(const _RealVector2<value_t>& vector) const
    {
        _RealVector4<value_t> result = {vector.x(), vector.y(), value_t(0), value_t(1)};
        transform(result);
        return _RealVector2<value_t>(result.x(), result.y());
    }

    /** Transforms a given value and returns a new instance. */
    template <typename T>
    T transform(const T& value) const { return detail::transform3(*this, value); }

    /** Transforms a given value in-place.
     * Modifies the input but also returns a reference to it.
     */
    template <typename T>
    T& transform(T& value) const { return detail::transform3(*this, value); }
};

//*********************************************************************************************************************/

using Xform3f = _Xform3<float>;
using Xform3d = _Xform3<double>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Xform into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
template <typename REAL>
std::ostream& operator<<(std::ostream& out, const notf::_Xform3<REAL>& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_Aabr. */
template <typename Real>
struct hash<notf::_Xform3<Real>> {
    size_t operator()(const notf::_Xform3<Real>& xform3) const { return notf::hash(xform3[0], xform3[1], xform3[2], xform3[3]); }
};
}

#ifndef NOTF_NO_SIMD
#include "common/simd/simd_xform3.hpp"
#endif
