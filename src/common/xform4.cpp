#include "common/xform4.hpp"

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

/* Xform4f ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform4f& mat)
{
    return out << "Xform4f(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Xform4f) == sizeof(float) * 16,
              "This compiler seems to inject padding bits into the notf::Xform4f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform4f>::value,
              "This compiler does not recognize notf::Xform4f as a POD.");

/* Xform4d ************************************************************************************************************/

template <>
std::ostream& operator<<(std::ostream& out, const Xform4d& mat)
{
    return out << "Xform4d(\n"
               << "\t" << mat[0][0] << ",\t" << mat[1][0] << ",\t" << mat[2][0] << ",\t" << mat[3][0] << "\n"
               << "\t" << mat[0][1] << ",\t" << mat[1][1] << ",\t" << mat[2][1] << ",\t" << mat[3][1] << "\n"
               << "\t" << mat[0][2] << ",\t" << mat[1][2] << ",\t" << mat[2][2] << ",\t" << mat[3][2] << "\n"
               << "\t" << mat[0][3] << ",\t" << mat[1][3] << ",\t" << mat[2][3] << ",\t" << mat[3][3] << ")";
}

static_assert(sizeof(Xform4d) == sizeof(double) * 16,
              "This compiler seems to inject padding bits into the notf::Xform4d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform4d>::value,
              "This compiler does not recognize notf::Xform4d as a POD.");

} // namespace notf
