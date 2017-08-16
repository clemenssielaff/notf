#include "common/xform3.hpp"

#include <iostream>

namespace notf {

//Transform3 Transform3::perspective(const float fov, const float aspectRatio, const float znear, const float zfar)
//{
//    const float inv_depth = 1.0f / (zfar - znear);
//    const float factor    = 1.0f / tan(fov / 2);

//    Transform3 result = Transform3::zero();
//    result.matrix[0]  = (1.0f / aspectRatio) * factor;
//    result.matrix[5]  = factor;
//    result.matrix[10] = -(zfar + znear) * inv_depth;
//    result.matrix[11] = -1.0f;
//    result.matrix[14] = -2.0f * zfar * znear * inv_depth;
//    return result;
//}

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
