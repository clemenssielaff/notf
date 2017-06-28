#include "common/xform4.hpp"

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

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Xform4f) == sizeof(float) * 16,
              "This compiler seems to inject padding bits into the notf::Xform4f memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform4f>::value,
              "This compiler does not recognize notf::Xform4f as a POD.");

static_assert(sizeof(Xform4d) == sizeof(double) * 16,
              "This compiler seems to inject padding bits into the notf::Xform4d memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Xform4d>::value,
              "This compiler does not recognize notf::Xform4d as a POD.");

} // namespace notf
