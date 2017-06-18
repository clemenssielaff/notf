#pragma once

#include <emmintrin.h>

#include "common/arithmetic.hpp"

namespace notf {
namespace detail {

template <typename SPECIALIZATION>
struct Arithmetic<SPECIALIZATION, float, 4, false> : public Arithmetic<SPECIALIZATION, float, 4, true> {

    using super = Arithmetic<SPECIALIZATION, float, 4, true>;

    Arithmetic() = default;

    /** Perfect forwarding constructor. */
    template <typename... T>
    Arithmetic(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    // SIMD specializations *******************************************************************************************/

    /** The sum of this value with another one. */
    SPECIALIZATION operator+(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(result.as_ptr(), _mm_add_ps(a, b));
        return result;
    }

    /** In-place addition of another value. */
    SPECIALIZATION& operator+=(const SPECIALIZATION& other)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(this->as_ptr(), _mm_add_ps(a, b));
        return super::_specialized_this();
    }

    /** The difference between this value and another one. */
    SPECIALIZATION operator-(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(result.as_ptr(), _mm_sub_ps(a, b));
        return result;
    }

    /** In-place subtraction of another value. */
    SPECIALIZATION& operator-=(const SPECIALIZATION& other)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(this->as_ptr(), _mm_sub_ps(a, b));
        return super::_specialized_this();
    }

    /** Component-wise multiplication of this value with another. */
    SPECIALIZATION operator*(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(result.as_ptr(), _mm_mul_ps(a, b));
        return result;
    }

    /** In-place component-wise multiplication with another value. */
    SPECIALIZATION& operator*=(const SPECIALIZATION& other)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(this->as_ptr(), _mm_mul_ps(a, b));
        return super::_specialized_this();
    }

    /** Multiplication of this value with a scalar. */
    SPECIALIZATION operator*(const float factor) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_load1_ps(&factor);
        _mm_storeu_ps(result.as_ptr(), _mm_mul_ps(a, b));
        return result;
    }

    /** In-place multiplication with a scalar. */
    SPECIALIZATION& operator*=(const float factor)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_load1_ps(&factor);
        _mm_storeu_ps(this->as_ptr(), _mm_mul_ps(a, b));
        return super::_specialized_this();
    }

    /** Component-wise division of this value by another. */
    SPECIALIZATION operator/(const SPECIALIZATION& other) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(result.as_ptr(), _mm_div_ps(a, b));
        return result;
    }

    /** In-place component-wise division by another value. */
    SPECIALIZATION& operator/=(const SPECIALIZATION& other)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_loadu_ps(other.as_ptr());
        _mm_storeu_ps(this->as_ptr(), _mm_div_ps(a, b));
        return super::_specialized_this();
    }

    /** Division of this value by a scalar. */
    SPECIALIZATION operator/(const float divisor) const
    {
        SPECIALIZATION result;
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_load1_ps(&divisor);
        _mm_storeu_ps(result.as_ptr(), _mm_div_ps(a, b));
        return result;
    }

    /** In-place division by a scalar. */
    SPECIALIZATION& operator/=(const float divisor)
    {
        const auto a = _mm_loadu_ps(this->as_ptr());
        const auto b = _mm_load1_ps(&divisor);
        _mm_storeu_ps(this->as_ptr(), _mm_div_ps(a, b));
        return super::_specialized_this();
    }
};

} // namespace detail
} // namespace notf
