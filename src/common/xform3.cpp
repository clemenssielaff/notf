#include "common/xform3.hpp"

#include <iostream>

namespace notf {

/* Xform3f ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform3f& mat)
{
    return out << "Xform3f(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Xform3f) == sizeof(float) * 16,
              "This compiler seems to inject padding bits into the notf::Xform3f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform3f>::value,
              "This compiler does not recognize notf::Xform3f as a POD.");

/* Xform3d ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform3d& mat)
{
    return out << "Xform3d(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Xform3d) == sizeof(double) * 16,
              "This compiler seems to inject padding bits into the notf::Xform3d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform3d>::value,
              "This compiler does not recognize notf::Xform3d as a POD.");

} // namespace notf
