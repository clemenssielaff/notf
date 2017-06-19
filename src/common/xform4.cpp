#include "common/xform4.hpp"

namespace notf {

Transform3 Transform3::perspective(const float fov, const float aspectRatio, const float znear, const float zfar)
{
    const float inv_depth = 1.0f / (zfar - znear);
    const float factor    = 1.0f / tan(fov / 2);

    Transform3 result = Transform3::zero();
    result.matrix[0]  = (1.0f / aspectRatio) * factor;
    result.matrix[5]  = factor;
    result.matrix[10] = -(zfar + znear) * inv_depth;
    result.matrix[11] = -1.0f;
    result.matrix[14] = -2.0f * zfar * znear * inv_depth;
    return result;
}

Transform3 Transform3::orthographic(const float width, const float height, const float znear, const float zfar)
{
    Transform3 result = Transform3::identity();
    if (width <= 0 || height <= 0 || znear == approx(zfar)) {
        return result;
    }

    result.matrix[0]  = 2.f / width;
    result.matrix[5]  = 2.f / height;
    result.matrix[10] = -2.f / (zfar - znear);
    result.matrix[14] = -(zfar + znear) / (zfar - znear);
    return result;
}

Transform3& Transform3::operator*=(const Transform3& other)
{
    *this = {{{matrix[0] * other.matrix[0] + matrix[4] * other.matrix[1] + matrix[8] * other.matrix[2] + matrix[12] * other.matrix[3],
               matrix[1] * other.matrix[0] + matrix[5] * other.matrix[1] + matrix[9] * other.matrix[2] + matrix[13] * other.matrix[3],
               matrix[2] * other.matrix[0] + matrix[6] * other.matrix[1] + matrix[10] * other.matrix[2] + matrix[14] * other.matrix[3],
               matrix[3] * other.matrix[0] + matrix[7] * other.matrix[1] + matrix[11] * other.matrix[2] + matrix[15] * other.matrix[3],

               matrix[0] * other.matrix[4] + matrix[4] * other.matrix[5] + matrix[8] * other.matrix[6] + matrix[12] * other.matrix[7],
               matrix[1] * other.matrix[4] + matrix[5] * other.matrix[5] + matrix[9] * other.matrix[6] + matrix[13] * other.matrix[7],
               matrix[2] * other.matrix[4] + matrix[6] * other.matrix[5] + matrix[10] * other.matrix[6] + matrix[14] * other.matrix[7],
               matrix[3] * other.matrix[4] + matrix[7] * other.matrix[5] + matrix[11] * other.matrix[6] + matrix[15] * other.matrix[7],

               matrix[0] * other.matrix[8] + matrix[4] * other.matrix[9] + matrix[8] * other.matrix[10] + matrix[12] * other.matrix[11],
               matrix[1] * other.matrix[8] + matrix[5] * other.matrix[9] + matrix[9] * other.matrix[10] + matrix[13] * other.matrix[11],
               matrix[2] * other.matrix[8] + matrix[6] * other.matrix[9] + matrix[10] * other.matrix[10] + matrix[14] * other.matrix[11],
               matrix[3] * other.matrix[8] + matrix[7] * other.matrix[9] + matrix[11] * other.matrix[10] + matrix[15] * other.matrix[11],

               matrix[0] * other.matrix[12] + matrix[4] * other.matrix[13] + matrix[8] * other.matrix[14] + matrix[12] * other.matrix[15],
               matrix[1] * other.matrix[12] + matrix[5] * other.matrix[13] + matrix[9] * other.matrix[14] + matrix[13] * other.matrix[15],
               matrix[2] * other.matrix[12] + matrix[6] * other.matrix[13] + matrix[10] * other.matrix[14] + matrix[14] * other.matrix[15],
               matrix[3] * other.matrix[12] + matrix[7] * other.matrix[13] + matrix[11] * other.matrix[14] + matrix[15] * other.matrix[15]}}};
    return *this;
}

/**
 * Compile-time sanity check.
 */
static_assert(sizeof(Transform3) == sizeof(float) * 16,
              "This compiler seems to inject padding bits into the notf::Transform3 memory layout. "
              "You should use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Transform3>::value,
              "This compiler does not recognize notf::Transform3 as a POD.");

} // namespace notf
