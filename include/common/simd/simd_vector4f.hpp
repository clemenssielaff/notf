/**
 * SIMD functions taken from:
 * https://github.com/g-truc/glm/blob/master/glm/simd/geometric.h
 */
#pragma once

#include <emmintrin.h>

#include "common/vector4.hpp"

namespace notf {

template <>
struct _RealVector4<float, true, false> : public _RealVector4<float, true, true> {

    using super = _RealVector4<float, true, true>;

    _RealVector4() = default;

    /** Perfect forwarding constructor. */
    template <typename... T>
    _RealVector4(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    // SIMD specializations *******************************************************************************************/

    /** Returns the dot product of this Vector4 and another.
     * Allows calculation of the magnitude of one vector in the direction of another.
     * Can be used to determine in which general direction a Vector4 is positioned
     * in relation to another one.
     * @param other     Other Vector4.
     */
    value_t dot(const _RealVector4& other) const
    {
        return _simd_vec4_dot(other)[0];
    }

    /** Vector3 cross product.
     * The cross product is a Vector3 perpendicular to this one and other.
     * The magnitude of the cross Vector3 is twice the area of the triangle
     * defined by the two input vectors.
     * The cross product is only defined for 3-dimensional vectors, the `w` element of the result will always be 1.
     * @param other     Other Vector3.
     */
    _RealVector4 get_crossed(const _RealVector4& other) const
    {
        const auto v1     = _mm_loadu_ps(this->as_ptr());
        const auto v2     = _mm_loadu_ps(other.as_ptr());
        const __m128 swp0 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 0, 2, 1));
        const __m128 swp1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 1, 0, 2));
        const __m128 swp2 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 0, 2, 1));
        const __m128 swp3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 1, 0, 2));
        const __m128 mul0 = _mm_mul_ps(swp0, swp3);
        const __m128 mul1 = _mm_mul_ps(swp1, swp2);
        const __m128 sub0 = _mm_sub_ps(mul0, mul1);

        _RealVector4 result;
        _mm_storeu_ps(result.as_ptr(), sub0);
        result.w() = 1.f;
        return result;
    }

private: // methods
    __m128 _simd_vec4_dot(const _RealVector4& other) const
    {
        const auto a      = _mm_loadu_ps(this->as_ptr());
        const auto b      = _mm_loadu_ps(other.as_ptr());
        const __m128 mul0 = _mm_mul_ps(a, b);
        const __m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
        const __m128 add0 = _mm_add_ps(mul0, swp0);
        const __m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
        const __m128 add1 = _mm_add_ps(add0, swp1);
        return add1;
    }
};

} // namespace notf
