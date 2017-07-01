#include "common/xform2.hpp"

#include <iostream>

namespace notf {

/* Xform2f ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform2f& mat)
{
    return out << "Xform2f(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << "\n"
               << "\t0,\t0,\t1\n)";
}

static_assert(sizeof(Xform2f) == sizeof(float) * 6,
              "This compiler seems to inject padding bits into the notf::Xform2f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform2f>::value,
              "This compiler does not recognize notf::Xform2f as a POD.");

/* Xform2d ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform2d& mat)
{
    return out << "Xform2d(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << "\n"
               << "\t0,\t0,\t1\n)";
}

static_assert(sizeof(Xform2d) == sizeof(double) * 6,
              "This compiler seems to inject padding bits into the notf::Xform2d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform2d>::value,
              "This compiler does not recognize notf::Xform2f as a POD.");

} // namespace notf
