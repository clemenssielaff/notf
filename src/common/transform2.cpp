#include "common/transform2.hpp"

#include <iostream>
#include <type_traits>

namespace notf {

Transform2& Transform2::operator*=(const Transform2& other)
{
    float* this_array        = this->as_ptr();
    const float* other_array = other.as_ptr();
    float t0                 = this_array[0] * other_array[0] + this_array[1] * other_array[2];
    float t2                 = this_array[2] * other_array[0] + this_array[3] * other_array[2];
    float t4                 = this_array[4] * other_array[0] + this_array[5] * other_array[2] + other_array[4];
    this_array[1]            = this_array[0] * other_array[1] + this_array[1] * other_array[3];
    this_array[3]            = this_array[2] * other_array[1] + this_array[3] * other_array[3];
    this_array[5]            = this_array[4] * other_array[1] + this_array[5] * other_array[3] + other_array[5];
    this_array[0]            = t0;
    this_array[2]            = t2;
    this_array[4]            = t4;
    return *this;
}

Transform2& Transform2::invert()
{
    // calculate the inverse with double precision
    const std::array<double, 6> t{
        {static_cast<double>(rows[0][0]),
         static_cast<double>(rows[0][1]),
         static_cast<double>(rows[1][0]),
         static_cast<double>(rows[1][1]),
         static_cast<double>(rows[2][0]),
         static_cast<double>(rows[2][1])}};
    const double det = t[0] * t[3] - t[2] * t[1];
    if (det > -1e-6 && det < 1e-6) {
        *this = Transform2::identity();
    }
    const double invdet = 1.0 / det;
    float* this_array   = &rows[0][0];
    this_array[0]       = static_cast<float>(t[3] * invdet);
    this_array[2]       = static_cast<float>(-t[2] * invdet);
    this_array[4]       = static_cast<float>((t[2] * t[5] - t[3] * t[4]) * invdet);
    this_array[1]       = static_cast<float>(-t[1] * invdet);
    this_array[3]       = static_cast<float>(t[0] * invdet);
    this_array[5]       = static_cast<float>((t[1] * t[4] - t[0] * t[5]) * invdet);
    return *this;
}

Transform2 Transform2::inverse() const
{
    const float det = rows[0][0] * rows[1][1] - rows[1][0] * rows[0][1];
    if (det > -1e-6f && det < 1e-6f) {
        return Transform2::identity();
    }
    const float invdet = 1.f / det;
    Transform2 result;
    result[0][0] = rows[1][1] * invdet;
    result[1][0] = -rows[1][0] * invdet;
    result[2][0] = (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]) * invdet;
    result[0][1] = -rows[0][1] * invdet;
    result[1][1] = rows[0][0] * invdet;
    result[2][1] = (rows[0][1] * rows[2][0] - rows[0][0] * rows[2][1]) * invdet;
    return result;
}

std::ostream& operator<<(std::ostream& out, const Transform2& mat)
{
    return out << "Transform2(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << "\n"
               << "\t0,\t0,\t1\n)";
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Transform2) == sizeof(float) * 6,
              "This compiler seems to inject padding bits into the notf::Transform2 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Transform2>::value,
              "This compiler does not recognize notf::Transform2 as a POD.");

} // namespace notf
