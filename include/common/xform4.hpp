#pragma once

#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "common/xform2.hpp"

#include <emmintrin.h>

namespace notf {

/** A full 3D Transformation Matrix with 4x4 components.
 * [a, e, i, m
 *  b, f, j, n
 *  c, g, k, o
 *  d, h, l, p]
 * Matrix layout is equivalent to glm's layout, which in turn is equivalent to GLSL's matrix layout for easy
 * compatiblity with OpenGL.
 */
template <typename Real, bool BASE_FOR_PARTIAL = false, ENABLE_IF_REAL(Real)>
struct _Xform4 : public detail::Arithmetic<_Xform4<Real, BASE_FOR_PARTIAL, true>, _RealVector4<Real>, 4, BASE_FOR_PARTIAL> {

    // explitic forwards
    using super    = detail::Arithmetic<_Xform4<Real>, _RealVector4<Real>, 4, BASE_FOR_PARTIAL>;
    using vector_t = _RealVector4<Real>;
    using value_t  = Real;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Xform4() = default;

    /** Value constructor defining the diagonal of the matrix. */
    explicit _Xform4(const value_t a)
        : super{vector_t{a, value_t(0), value_t(0), value_t(0)},
                vector_t{value_t(0), a, value_t(0), value_t(0)},
                vector_t{value_t(0), value_t(0), a, value_t(0)},
                vector_t{value_t(0), value_t(0), value_t(0), a}}
    {
    }

    /** Column-wise constructor of the matrix. */
    _Xform4(const vector_t a, const vector_t b, const vector_t c, const vector_t d)
        : super{std::move(a), std::move(b), std::move(c), std::move(d)}
    {
    }

    /** Element-wise constructor. */
    _Xform4(const value_t a, const value_t b, const value_t c, const value_t d,
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
    static _Xform4 identity() { return _Xform4(value_t(1)); }

    static _Xform4 translation(const Vector2f& t)
    {
        return _Xform4{vector_t(value_t(1), value_t(0), value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(1), value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(0), value_t(1), value_t(0)),
                       vector_t(t.x(), t.y(), value_t(0), value_t(1))};
    }

    static _Xform4 translation(const Vector4f& t)
    {
        return _Xform4{vector_t(value_t(1), value_t(0), value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(1), value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(0), value_t(1), value_t(0)),
                       vector_t(t.x(), t.y(), t.z(), value_t(1))};
    }

    static _Xform4 rotation(const value_t radian, const vector_t axis)
    {
        return identity().get_rotated(radian, std::move(axis));
    }

    static _Xform4 scaling(const value_t scale)
    {
        return _Xform4{vector_t(scale, value_t(0), value_t(0), value_t(0)),
                       vector_t(value_t(0), scale, value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(0), scale, value_t(0)),
                       vector_t(value_t(0), value_t(0), value_t(0), value_t(1))};
    }

    static _Xform4 scaling(const vector_t& scale)
    {
        return _Xform4{vector_t(scale[value_t(0)], value_t(0), value_t(0), value_t(0)),
                       vector_t(value_t(0), scale[value_t(1)], value_t(0), value_t(0)),
                       vector_t(value_t(0), value_t(0), scale[2], value_t(0)),
                       vector_t(value_t(0), value_t(0), value_t(0), value_t(1))};
    }

    //    /** Creates a perspective transformation.
    //     * @param fov           Horizontal field of view in radians.
    //     * @param aspectRatio   Aspect ratio (width / height)
    //     * @param znear         Distance to the near plane in z direction.
    //     * @param zfar          Distance to the far plane in z direction.
    //     */
    //    static Transform3 perspective(const float fov, const float aspectRatio, const float znear, const float zfar);

    //    static Transform3 orthographic(const float width, const float height, const float znear, const float zfar);

    /* Inspection *****************************************************************************************************/

    /** Returns the translation component of this Xform. */
    const vector_t& get_translation() const { return data[3]; }

    /* Modification ***************************************************************************************************/

    /** Concatenation of two transformation matrices. */
    _Xform4 operator*(const _Xform4& other) const
    {
        _Xform4 result;
        result[0] = data[0] * other[0][0] + data[1] * other[0][1] + data[2] * other[0][2] + data[3] * other[0][3];
        result[1] = data[0] * other[1][0] + data[1] * other[1][1] + data[2] * other[1][2] + data[3] * other[1][3];
        result[2] = data[0] * other[2][0] + data[1] * other[2][1] + data[2] * other[2][2] + data[3] * other[2][3];
        result[3] = data[0] * other[3][0] + data[1] * other[3][1] + data[2] * other[3][2] + data[3] * other[3][3];
        return result;
    }

    /** Applies the other Xform to this one in-place. */
    _Xform4& operator*=(const _Xform4& other)
    {
        *this = *this * other;
        return *this;
    }

    /** Premultiplies the other transform matrix with this one in-place. */
    _Xform4& premult(const _Xform4& other)
    {
        *this = other * *this;
        return *this;
    }

    /** Copy of this transform with an additional translation. */
    _Xform4 get_translated(const vector_t& delta) const
    {
        _Xform4 Result;
        Result[0] = data[0];
        Result[1] = data[1];
        Result[2] = data[2];
        Result[3] = data[0] * delta[0] + data[1] * delta[1] + data[2] * delta[2] + data[3];
        return Result;
    }

    /** Applies a right-hand rotation around the given axis to this xform. */
    _Xform4& translate(const vector_t& delta)
    {
        data[3] = data[0] * delta[0] + data[1] * delta[1] + data[2] * delta[2] + data[3];
        return *this;
    }

    /** Copy of this transform with an additional right-hand rotation around the given axis. */
    _Xform4 get_rotated(const value_t radian, vector_t axis) const
    {
        const value_t cos_angle = cos(radian);
        const value_t sin_angle = sin(radian);

        axis[3] = 0;
        axis.normalize();
        const vector_t temp = axis * (1 - cos_angle);

        _Xform4 rotation;
        rotation[0][0] = cos_angle + temp[0] * axis[0];
        rotation[0][1] = temp[0] * axis[1] + sin_angle * axis[2];
        rotation[0][2] = temp[0] * axis[2] - sin_angle * axis[1];

        rotation[1][0] = temp[1] * axis[0] - sin_angle * axis[2];
        rotation[1][1] = cos_angle + temp[1] * axis[1];
        rotation[1][2] = temp[1] * axis[2] + sin_angle * axis[0];

        rotation[2][0] = temp[2] * axis[0] + sin_angle * axis[1];
        rotation[2][1] = temp[2] * axis[1] - sin_angle * axis[0];
        rotation[2][2] = cos_angle + temp[2] * axis[2];

        _Xform4 result;
        result[0] = data[0] * rotation[0][0] + data[1] * rotation[0][1] + data[2] * rotation[0][2];
        result[1] = data[0] * rotation[1][0] + data[1] * rotation[1][1] + data[2] * rotation[1][2];
        result[2] = data[0] * rotation[2][0] + data[1] * rotation[2][1] + data[2] * rotation[2][2];
        result[3] = data[3];
        return result;
    }

    /** Applies a right-hand rotation around the given axis to this xform. */
    _Xform4& rotate(const value_t radian, vector_t axis)
    {
        *this = get_rotated(radian, std::move(axis));
        return *this;
    }

    /** Copy of this transform with an additional non-uniform scaling. */
    _Xform4 get_scaled(const vector_t& factor) const
    {
        _Xform4 Result;
        Result[0] = data[0] * factor[0];
        Result[1] = data[1] * factor[1];
        Result[2] = data[2] * factor[2];
        Result[3] = data[3];
        return Result;
    }

    /** Applies a non-uniform scaling to this xform. */
    _Xform4& scale(const vector_t& factor) const
    {
        data[0] *= factor[0];
        data[1] *= factor[1];
        data[2] *= factor[2];
        return *this;
    }

    /** Copy of this transform with an additional non-uniform scaling. */
    _Xform4 get_scaled(const value_t& factor) const
    {
        _Xform4 Result;
        Result[0] = data[0] * factor;
        Result[1] = data[1] * factor;
        Result[2] = data[2] * factor;
        Result[3] = data[3];
        return Result;
    }

    /** Applies a non-uniform scaling to this xform. */
    _Xform4& scale(const value_t& factor) const
    {
        data[0] *= factor;
        data[1] *= factor;
        data[2] *= factor;
        return *this;
    }

    /** Returns the inverse of this Xform2. */
    _Xform4 get_inverse() const
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

        vector_t signA(value_t(+1), value_t(-1), value_t(+1), value_t(-1));
        vector_t signB(value_t(-1), value_t(+1), value_t(-1), value_t(+1));
        _Xform4 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

        vector_t row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);
        vector_t dot0(data[0] * row0);
        value_t dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

        return inverse / dot1;
    }

    /** Inverts this Xform in-place. */
    _Xform4& invert()
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
        //        const _RealVector4<value_t> mov0 = _RealVector4<value_t>::fill(vector[0]);
        //        const _RealVector4<value_t> mov1 = _RealVector4<value_t>::fill(vector[1]);
        //        const _RealVector4<value_t> mul0 = data[0] * mov0;
        //        const _RealVector4<value_t> mul1 = data[1] * mov1;
        //        const _RealVector4<value_t> mov2 = _RealVector4<value_t>::fill(vector[2]);
        //        const _RealVector4<value_t> mov3 = _RealVector4<value_t>::fill(vector[3]);
        //        const _RealVector4<value_t> mul2 = data[2] * mov2;
        //        const _RealVector4<value_t> mul3 = data[3] * mov3;

        //        const _RealVector4<value_t> add0 = mul0 + mul1;
        //        const _RealVector4<value_t> add1 = mul2 + mul3;

        //        vector = add0 + add1;
        //        return vector;

        // this is the operation vector * matrix
        vector = vector_t(
            data[0][0] * vector[0] + data[0][1] * vector[1] + data[0][2] * vector[2] + data[0][3] * vector[3],
            data[1][0] * vector[0] + data[1][1] * vector[1] + data[1][2] * vector[2] + data[1][3] * vector[3],
            data[2][0] * vector[0] + data[2][1] * vector[1] + data[2][2] * vector[2] + data[2][3] * vector[3],
            data[3][0] * vector[0] + data[3][1] * vector[1] + data[3][2] * vector[2] + data[3][3] * vector[3]);
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

    _Xform2<value_t> to_xform2() const
    {
        const auto t = get_translation(); // TODO: _Xform4::to_xform2();
        return _Xform2<value_t>::translation(_RealVector2<value_t>(t.x(), t.y()));
    }
};

using Xform4f = _Xform4<float>;
using Xform4d = _Xform4<double>;

} // namespace notf

#ifndef NOTF_NO_SIMD
#include "common/simd/simd_xform4.hpp"
#endif
