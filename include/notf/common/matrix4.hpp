#pragma once

#include "notf/common/vector4.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// matrix4 ========================================================================================================== //

/// A 3D Transformation matrix with 4x4 components.
///
/// [a, e, i, m
///  b, f, j, n
///  c, g, k, o
///  d, h, l, p]
///
/// Matrix layout is equivalent to glm's layout, which in turn is equivalent to GLSL's matrix layout for easy
/// compatiblity with OpenGL.

template<typename Element>
struct Matrix4 : public Arithmetic<Matrix4<Element>, Element, Vector4<Element>, 4> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = Arithmetic<Matrix4<Element>, Element, Vector4<Element>, 4>;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Component type used by this arithmetic type.
    using component_t = typename super_t::component_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Matrix4() noexcept = default;

    /// Value constructor defining the diagonal of the matrix.
    /// @param a    Value to put into the diagonal.
    explicit Matrix4(const element_t a)
        : super_t(component_t(a, 0, 0, 0), component_t(0, a, 0, 0), component_t(0, 0, a, 0), component_t(0, 0, 0, a)) {}

    /// Column-wise constructor of the matrix.
    /// @param a    First column.
    /// @param b    Second column.
    /// @param c    Third column.
    /// @param d    Fourth column.
    Matrix4(const component_t a, const component_t b, const component_t c, const component_t d)
        : super_t(std::move(a), std::move(b), std::move(c), std::move(d)) {}

    /// Element-wise constructor.
    Matrix4(const element_t a, const element_t b, const element_t c, const element_t d, const element_t e,
            const element_t f, const element_t g, const element_t h, const element_t i, const element_t j,
            const element_t k, const element_t l, const element_t m, const element_t n, const element_t o,
            const element_t p)
        : super_t(component_t(a, b, c, d), component_t(e, f, g, h), component_t(i, j, k, l), component_t(m, n, o, p)) {}

    /// The identity matrix.
    static Matrix4 identity() { return Matrix4(1); }

    /// An element-wise translation matrix.
    /// @param x    X transformation.
    /// @param y    Y transformation.
    /// @param z    Z transformation (default to 0).
    static Matrix4 translation(const element_t x, const element_t y, const element_t z = 0) {
        return Matrix4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
    }

    /// NAme of this Matrix4 type.
    static constexpr const char* get_name() {
        if constexpr (std::is_same_v<Element, float>) {
            return "M4f";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "M4d";
        }
    }

    //    /// A 2d translation matrix.
    //    /// @param translation  2D translation vector.
    //    static Matrix4 translation(const Vector2<element_t>& t) { return Matrix4::translation(t.x(), t.y()); }

    //    /// A 3d translation matrix.
    //    /// @param translation  3D translation vector.
    //    static Matrix4 translation(const Vector3<element_t>& t) { return Matrix4::translation(t.x(), t.y(), t.z()); }

    //    /// A rotation matrix.
    //    /// @param axis     Rotation axis.
    //    /// @param radians  Rotation in radians.
    //    static Matrix4 rotation(const Vector3<element_t> axis, const element_t radians)
    //    {
    //        return identity().rotate(std::move(axis), radians);
    //    }

    /// A uniform scale matrix.
    /// @param factor   Uniform scale factor.
    static Matrix4 scaling(const element_t s) { return Matrix4(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, 0, 0, 0, 1); }

    /// A non-uniform scale matrix.
    /// @param x    X component of the scale vector.
    /// @param y    Y component of the scale vector.
    /// @param z    Z component of the scale vector.
    static Matrix4 scaling(const element_t x, const element_t y, const element_t z = 1) {
        return Matrix4(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1);
    }

    /// Creates a perspective transformation.
    /// @param fov       Horizontal field of view in radians.
    /// @param aspect    Aspect ratio (width / height)
    /// @param znear     Distance to the near plane in z direction.
    /// @param zfar      Distance to the far plane in z direction.
    static Matrix4 perspective(const element_t fov, const element_t aspect, element_t near, element_t far) {
        // near and far planes must be >= 1
        near = max(near, 1);
        far = max(near, far);

        Matrix4 result = Matrix4::zero();
        if (std::abs(aspect) <= precision_high<element_t>() || std::abs(far - near) <= precision_high<element_t>()) {
            return result;
        }

        const element_t tan_half_fov = tan(fov / 2);

        result[0][0] = 1 / (aspect * tan_half_fov);
        result[1][1] = 1 / tan_half_fov;
        result[2][3] = -1;
        result[2][2] = -(far + near) / (far - near);
        result[3][2] = -(2 * far * near) / (far - near);

        return result;
    }

    /// Creates an orthographic transformation matrix.
    /// @param left     Vertical distance from screen zero to the left edge of the projection.
    /// @param right    Vertical distance from screen zero to the right edge of the projection.
    /// @param bottom   Vertical distance from screen zero to the bottom edge of the projection.
    /// @param top      Vertical distance from screen zero to the top edge of the projection.
    /// @param znear    Distance to the near plane in z direction.
    /// @param zfar     Distance to the far plane in z direction.
    static Matrix4 orthographic(const element_t left, const element_t right, const element_t bottom,
                                const element_t top, element_t near, element_t far) {
        // near and far planes must be >= 1
        near = notf::max(near, 1);
        far = notf::max(near, far);

        const element_t width = right - left;
        const element_t height = top - bottom;
        const element_t depth = far - near;

        Matrix4 result = identity();
        if (std::abs(width) <= precision_high<element_t>() || std::abs(height) <= precision_high<element_t>()
            || std::abs(depth) <= precision_high<element_t>()) {
            return result;
        }

        result[0][0] = 2 / width;
        result[1][1] = 2 / height;
        result[3][0] = -(right + left) / width;
        result[3][1] = -(top + bottom) / height;
        result[2][2] = -2 / depth;
        result[3][2] = -(near + far) / depth;

        return result;
    }

    /// The translation part of this matrix.
    const component_t& translation() const { return data[3]; }

    /// Concatenation of two transformation matrices.
    /// @param other    Transformation to concatenate.
    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        result[0] = data[0] * other[0][0] + data[1] * other[0][1] + data[2] * other[0][2] + data[3] * other[0][3];
        result[1] = data[0] * other[1][0] + data[1] * other[1][1] + data[2] * other[1][2] + data[3] * other[1][3];
        result[2] = data[0] * other[2][0] + data[1] * other[2][1] + data[2] * other[2][2] + data[3] * other[2][3];
        result[3] = data[0] * other[3][0] + data[1] * other[3][1] + data[2] * other[3][2] + data[3] * other[3][3];
        return result;
    }

    /// Concatenation of another transformation matrix to this one in-place.
    /// @param other    Transformation to concatenate.
    Matrix4& operator*=(const Matrix4& other) {
        *this = *this * other;
        return *this;
    }

    /// Translates this transformation by a given delta vector.
    /// @param delta    Delta translation.
    Matrix4 translate(const component_t& delta) const {
        Matrix4 result;
        result[0] = data[0];
        result[1] = data[1];
        result[2] = data[2];
        result[3] = data[0] * delta[0] + data[1] * delta[1] + data[2] * delta[2] + data[3];
        return result;
    }

    //    /// Rotates the transformation by a given angle in radians over a given axis.
    //    /// @param axis     Rotation axis.
    //    /// @param radians  Rotation in radians.
    //    Matrix4 rotate(Vector3<element_t> axis, const element_t radian) const
    //    {
    //        const element_t cos_angle = cos(radian);
    //        const element_t sin_angle = sin(radian);

    //        axis.normalize();
    //        const Vector3<element_t> temp = axis * (1 - cos_angle);

    //        Matrix4 rotation;
    //        rotation[0][0] = cos_angle + temp[0] * axis[0];
    //        rotation[0][1] = temp[0] * axis[1] + sin_angle * axis[2];
    //        rotation[0][2] = temp[0] * axis[2] - sin_angle * axis[1];

    //        rotation[1][0] = temp[1] * axis[0] - sin_angle * axis[2];
    //        rotation[1][1] = cos_angle + temp[1] * axis[1];
    //        rotation[1][2] = temp[1] * axis[2] + sin_angle * axis[0];

    //        rotation[2][0] = temp[2] * axis[0] + sin_angle * axis[1];
    //        rotation[2][1] = temp[2] * axis[1] - sin_angle * axis[0];
    //        rotation[2][2] = cos_angle + temp[2] * axis[2];

    //        Matrix4 result;
    //        result[0] = data[0] * rotation[0][0] + data[1] * rotation[0][1] + data[2] * rotation[0][2];
    //        result[1] = data[0] * rotation[1][0] + data[1] * rotation[1][1] + data[2] * rotation[1][2];
    //        result[2] = data[0] * rotation[2][0] + data[1] * rotation[2][1] + data[2] * rotation[2][2];
    //        result[3] = data[3];

    //        return result;
    //    }

    /// Applies a non-uniform scaling to this matrix.
    /// scaling  Scaling vector.
    Matrix4 scale(const component_t& scaling) const {
        Matrix4 result = *this;
        result[0] *= scaling[0];
        result[1] *= scaling[1];
        result[2] *= scaling[2];
        return result;
    }

    /// Applies an uniform scaling to this matrix.
    /// scaling  Scale factor.
    Matrix4 scale(const element_t factor) const {
        Matrix4 result = *this;
        result[0] *= factor;
        result[1] *= factor;
        result[2] *= factor;
        return result;
    }

    /// Returns the inverse of this matrix.
    Matrix4 get_inverse() const {
        element_t coef00 = data[2][2] * data[3][3] - data[3][2] * data[2][3];
        element_t coef02 = data[1][2] * data[3][3] - data[3][2] * data[1][3];
        element_t coef03 = data[1][2] * data[2][3] - data[2][2] * data[1][3];

        element_t coef04 = data[2][1] * data[3][3] - data[3][1] * data[2][3];
        element_t coef06 = data[1][1] * data[3][3] - data[3][1] * data[1][3];
        element_t coef07 = data[1][1] * data[2][3] - data[2][1] * data[1][3];

        element_t coef08 = data[2][1] * data[3][2] - data[3][1] * data[2][2];
        element_t coef10 = data[1][1] * data[3][2] - data[3][1] * data[1][2];
        element_t coef11 = data[1][1] * data[2][2] - data[2][1] * data[1][2];

        element_t coef12 = data[2][0] * data[3][3] - data[3][0] * data[2][3];
        element_t coef14 = data[1][0] * data[3][3] - data[3][0] * data[1][3];
        element_t coef15 = data[1][0] * data[2][3] - data[2][0] * data[1][3];

        element_t coef16 = data[2][0] * data[3][2] - data[3][0] * data[2][2];
        element_t coef18 = data[1][0] * data[3][2] - data[3][0] * data[1][2];
        element_t coef19 = data[1][0] * data[2][2] - data[2][0] * data[1][2];

        element_t coef20 = data[2][0] * data[3][1] - data[3][0] * data[2][1];
        element_t coef22 = data[1][0] * data[3][1] - data[3][0] * data[1][1];
        element_t coef23 = data[1][0] * data[2][1] - data[2][0] * data[1][1];

        component_t fac0(coef00, coef00, coef02, coef03);
        component_t fac1(coef04, coef04, coef06, coef07);
        component_t fac2(coef08, coef08, coef10, coef11);
        component_t fac3(coef12, coef12, coef14, coef15);
        component_t fac4(coef16, coef16, coef18, coef19);
        component_t fac5(coef20, coef20, coef22, coef23);

        component_t vec0(data[1][0], data[0][0], data[0][0], data[0][0]);
        component_t vec1(data[1][1], data[0][1], data[0][1], data[0][1]);
        component_t vec2(data[1][2], data[0][2], data[0][2], data[0][2]);
        component_t vec3(data[1][3], data[0][3], data[0][3], data[0][3]);

        component_t inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
        component_t inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
        component_t inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
        component_t inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

        component_t signA(+1, -1, +1, -1);
        component_t signB(-1, +1, -1, +1);
        Matrix4 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

        component_t row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);
        component_t dot0(data[0] * row0);
        element_t dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

        return inverse / dot1;
    }

    /// Returns the transformed copy of a given vector.
    /// @param vector   Vector to transform.
    Vector4<element_t> transform(const Vector4<element_t>& vector) const {
        // this is the operation matrix * vector
        const Vector4<element_t> mul0 = data[0] * Vector4<element_t>::fill(vector[0]);
        const Vector4<element_t> mul1 = data[1] * Vector4<element_t>::fill(vector[1]);
        const Vector4<element_t> mul2 = data[2] * Vector4<element_t>::fill(vector[2]);
        const Vector4<element_t> mul3 = data[3] * Vector4<element_t>::fill(vector[3]);
        return (mul0 + mul1) + (mul2 + mul3);
    }

    //    /// Returns the transformed copy of a given vector.
    //    /// @param vector   Vector to transform.
    //    Vector2<element_t> transform(const Vector2<element_t>& vector) const
    //    {
    //        const Vector4<element_t> mul0 = data[0] * Vector4<element_t>::fill(vector[0]);
    //        const Vector4<element_t> mul1 = data[1] * Vector4<element_t>::fill(vector[1]);
    //        const Vector4<element_t> result = (mul0 + mul1) + data[3];
    //        return Vector2<element_t>(result.x(), result.y());
    //    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of a matrix into a std::ostream.
/// @param out      Output stream, implicitly passed with the << operator.
/// @param matrix   Matrix to print.
/// @return Output stream for further output.
template<typename Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Matrix4<Element>& mat) {
    return out << fmt::format("{}", mat);
}
NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Matrix4<Element>> {
    using type = notf::detail::Matrix4<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& mat, FormatContext& ctx) {
        return format_to(ctx.begin(),
                         "{}({: #7.6g}, {: #7.6g}, {: #7.6g}, {: #7.6g}\n"
                         "    {: #7.6g}, {: #7.6g}, {: #7.6g}, {: #7.6g}\n"
                         "    {: #7.6g}, {: #7.6g}, {: #7.6g}, {: #7.6g}\n"
                         "    {: #7.6g}, {: #7.6g}, {: #7.6g}, {: #7.6g})",
                         type::get_name(),                            //
                         mat[0][0], mat[1][0], mat[2][0], mat[3][0],  //
                         mat[0][1], mat[1][1], mat[2][1], mat[3][1],  //
                         mat[0][2], mat[1][2], mat[2][2], mat[3][2],  //
                         mat[0][3], mat[1][3], mat[2][3], mat[3][3]); //
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Matrix4.
template<class Element>
struct std::hash<notf::detail::Matrix4<Element>> {
    size_t operator()(const notf::detail::Matrix4<Element>& matrix) const {
        return notf::hash(notf::to_number(notf::detail::HashID::MATRIX4), //
                          matrix[0], matrix[1], matrix[2], matrix[3]);
    }
};

// compile time tests =============================================================================================== //

static_assert(notf::is_pod_v<notf::M4f>);
static_assert(notf::is_pod_v<notf::M4d>);

static_assert(sizeof(notf::M4f) == sizeof(float) * 16);
static_assert(sizeof(notf::M4d) == sizeof(double) * 16);
