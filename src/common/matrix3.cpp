#include "common/matrix3.hpp"

#include <iostream>

namespace notf {

// Matrix3f ==========================================================================================================//

template <>
std::ostream& operator<<(std::ostream& out, const Matrix3f& mat)
{
    return out << "Matrix3f(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << "\n"
               << "\t0,\t0,\t1\n)";
}

static_assert(sizeof(Matrix3f) == sizeof(float) * 6,
              "This compiler seems to inject padding bits into the notf::Matrix3f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Matrix3f>::value,
              "This compiler does not recognize notf::Matrix3f as a POD.");

// Matrix3d ==========================================================================================================//

template <>
std::ostream& operator<<(std::ostream& out, const Matrix3d& mat)
{
    return out << "Matrix3d(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << "\n"
               << "\t0,\t0,\t1\n)";
}

static_assert(sizeof(Matrix3d) == sizeof(double) * 6,
              "This compiler seems to inject padding bits into the notf::Matrix3d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Matrix3d>::value,
              "This compiler does not recognize notf::Matrix3d as a POD.");

} // namespace notf
