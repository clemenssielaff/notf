#pragma once

#include "common/vector4.hpp"

namespace notf {

/** A full 3D Transformation Matrix with 4x4 components.
 * [a, e, i, m
 *  b, f, j, n
 *  c, g, k, o
 *  d, h, l, p]
 * Matrix layout is equivalent to glm's layout, which in turn is equivalent to GLSL's matrix layout for easy
 * compatiblity with OpenGL.
 */
template <typename Real, ENABLE_IF_REAL(Real), bool BASE_FOR_PARTIAL = false>
struct _Xform4 : public detail::Arithmetic<_Xform4<Real>, _RealVector4<Real>, 4> {

    // explitic forwards
    using super    = detail::Arithmetic<_Xform4<Real>, _RealVector4<Real>, 4>;
    using vector_t = _RealVector4<Real>;
    using value_t  = Real;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Xform4() = default;

    /** Value constructor defining the diagonal of the matrix. */
    _Xform4(const value_t a)
        : super{vector_t{a, 0, 0, 0},
                vector_t{0, a, 0, 0},
                vector_t{0, 0, a, 0},
                vector_t{0, 0, 0, a}}
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

    static _Xform4 translation(const Vector4f& t)
    {
        return _Xform4{vector_t{1, 0, 0, 0},
                       vector_t{0, 1, 0, 0},
                       vector_t{0, 0, 1, 0},
                       vector_t{t.x(), t.y(), t.z(), 1}};
    }

    static _Xform4 rotation(const value_t radian, const vector_t axis)
    {
        return identity().get_rotated(radian, std::move(axis));
    }

    static _Xform4 scaling(const value_t scale)
    {
        return _Xform4{vector_t{scale, 0, 0, 0},
                       vector_t{0, scale, 0, 0},
                       vector_t{0, 0, scale, 0},
                       vector_t{0, 0, 0, 1}};
    }

    static _Xform4 scaling(const vector_t& scale)
    {
        return _Xform4{vector_t{scale[0], 0, 0, 0},
                       vector_t{0, scale[1], 0, 0},
                       vector_t{0, 0, scale[2], 0},
                       vector_t{0, 0, 0, 1}};
    }

    //    /** Creates a perspective transformation.
    //     * @param fov           Horizontal field of view in radians.
    //     * @param aspectRatio   Aspect ratio (width / height)
    //     * @param znear         Distance to the near plane in z direction.
    //     * @param zfar          Distance to the far plane in z direction.
    //     */
    //    static Transform3 perspective(const float fov, const float aspectRatio, const float znear, const float zfar);

    //    static Transform3 orthographic(const float width, const float height, const float znear, const float zfar);

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /** Returns the translation component of this Xform. */
    const vector_t& get_translation() const { return data[3]; }

    //  MODIFICATION ///////////////////////////////////////////////////////////////////////////////////////////////////

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
        *this = this * other;
        return *this;
    }

    /** Premultiplies the other transform matrix with this one in-place. */
    _Xform4& premult(const _Xform4& other)
    {
        *this = other * this;
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
        const value_t det = get_determinant();
        if (abs(det) <= precision_high<value_t>()) {
            return _Xform2::identity();
        }
        const value_t invdet = 1 / det;
        _Xform2 result;
        result[0][0] = +(rows[1][1]) * invdet;
        result[0][1] = -(rows[0][1]) * invdet;
        result[1][0] = -(rows[1][0]) * invdet;
        result[1][1] = +(rows[0][0]) * invdet;
        result[2][0] = +(rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]) * invdet;
        result[2][1] = -(rows[0][0] * rows[2][1] - rows[0][1] * rows[2][0]) * invdet;
        return result;
    }

    /** Inverts this Xform in-place. */
    _Xform4& invert()
    {
        *this = get_inverse();
        return *this;
    }
};

using Xform4f = _Xform4<float>;

} // namespace notf
