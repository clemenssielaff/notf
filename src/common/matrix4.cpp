#include "common/matrix4.hpp"

#include <iostream>

NOTF_OPEN_NAMESPACE

// Matrix4f ==========================================================================================================//

template<>
std::ostream& operator<<(std::ostream& out, const Matrix4f& mat)
{
    return out << "Matrix4f(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Matrix4f) == sizeof(float) * 16,
              "This compiler seems to inject padding bits into the notf::Matrix4f memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Matrix4f>::value, "This compiler does not recognize notf::Matrix4f as a POD.");

// Matrix4d ==========================================================================================================//

template<>
std::ostream& operator<<(std::ostream& out, const Matrix4d& mat)
{
    return out << "Matrix4d(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Matrix4d) == sizeof(double) * 16,
              "This compiler seems to inject padding bits into the notf::Matrix4d memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Matrix4d>::value, "This compiler does not recognize notf::Matrix4d as a POD.");

NOTF_CLOSE_NAMESPACE
