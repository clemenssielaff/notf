#pragma once

#include <array>
#include <iosfwd>

#include "common/float.hpp"
#include "common/vector3.hpp"

namespace notf {

/** A 3D Transformation Matrix with 4x4 components.
 * [[a, b, c, d]
 *  [e, f, g, h]
 *  [i, j, k, l]
 *  [m, n, o, p]]
 */
struct Transform3 {

    //  FIELDS  ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /** Matrix values. */
    mutable std::array<float, 16> matrix;

    //  HOUSEHOLD  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Do not implement the default methods, so this data structure remains a POD.
    Transform3()                        = default;            // Default Constructor.
    ~Transform3()                       = default;            // Destructor.
    Transform3(const Transform3& other) = default;            // Copy Constructor.
    Transform3& operator=(const Transform3& other) = default; // Assignment Operator.

    //  STATIC CONSTRUCTORS  //////////////////////////////////////////////////////////////////////////////////////////

    /** Zero matrix */
    static Transform3 zero()
    {
        return {{{0, 0, 0, 0,
                  0, 0, 0, 0,
                  0, 0, 0, 0,
                  0, 0, 0, 0}}};
    }

    /** Identity Transform */
    static Transform3 identity()
    {
        return {{{1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  0, 0, 0, 1}}};
    }

    static Transform3 translation(const Vector3f& t)
    {
        return {{{1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  t.x, t.y, t.z, 1}}};
    }

    static Transform3 rotation_y(float radianAngle)
    {
        const float c = cos(radianAngle);
        const float s = sin(radianAngle);
        return {{{c, 0, -s, 0,
                  0, 1, 0, 0,
                  s, 0, c, 0,
                  0, 0, 0, 1}}};
    }

    static Transform3 scale(const float scale)
    {
        return {{{scale, 0, 0, 0,
                  0, scale, 0, 0,
                  0, 0, scale, 0,
                  0, 0, 0, 1}}};
    }

    static Transform3 scale(const Vector3f& scale)
    {
        return {{{scale.x, 0, 0, 0,
                  0, scale.y, 0, 0,
                  0, 0, scale.z, 0,
                  0, 0, 0, 1}}};
    }

    /** Creates a perspective transformation.
     * @param fov           Horizontal field of view in radians.
     * @param aspectRatio   Aspect ratio (width / height)
     * @param znear         Distance to the near plane in z direction.
     * @param zfar          Distance to the far plane in z direction.
     */
    static Transform3 perspective(const float fov, const float aspectRatio, const float znear, const float zfar);

    static Transform3 orthographic(const float width, const float height, const float znear, const float zfar);

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /** Returns the translation component of this Transform3. */
    const Vector3f& get_translation() const { return *reinterpret_cast<Vector3f*>(&matrix[12]); }

    /** Allows direct read-only access to the Transform3's internal storage. */
    const float* as_ptr() const { return &matrix[0]; }

    /** Allows direct read/write access to the Transform3's internal storage. */
    float* as_ptr() { return &matrix[0]; }

    //  MODIFICATION ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Matrix multiplication of this Matrix with another.
    Transform3 operator*(const Transform3& other) const
    {
        Transform3 result = *this;
        result *= other;
        return result;
    }

    /** Applies the other Transform to this one in-place. */
    Transform3& operator*=(const Transform3& other);
};

} // namespace notf
