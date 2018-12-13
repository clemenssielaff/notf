#pragma once

#include "notf/common/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// matrix3 ========================================================================================================== //

/// A 2D transformation matrix with 3x3 components.
///
/// [a, c, e,
///  b, d, f
///  0, 0, 1]
///
/// Only the first two rows are actually stored though, the last row is implicit.
template<class Element>
struct Matrix3 : public Arithmetic<Matrix3<Element>, Element, Vector2<Element>, 3> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = Arithmetic<Matrix3<Element>, Element, Vector2<Element>, 3>;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Component type used by this arithmetic type.
    using component_t = typename super_t::component_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    Matrix3() = default;

    /// Value constructor defining the diagonal of the matrix.
    /// @param a    Value to put into the diagonal.
    explicit Matrix3(const element_t a) : super_t(component_t(a, 0), component_t(0, a), component_t(0, 0)) {}

    /// Column-wise constructor of the matrix.
    /// @param a    First column.
    /// @param b    Second column.
    /// @param c    Third column.
    Matrix3(component_t a, component_t b, component_t c) : super_t(std::move(a), std::move(b), std::move(c)) {}

    /// Element-wise constructor.
    Matrix3(const element_t a, const element_t b, const element_t c, const element_t d, const element_t e,
            const element_t f)
        : super_t(component_t(a, b), component_t(c, d), component_t(e, f)) {}

    /// The identity matrix.
    static Matrix3 identity() { return Matrix3(1); }

    /// A translation matrix.
    /// @param translation  Translation vector.
    static Matrix3 translation(component_t translation) {
        return Matrix3(component_t(1, 0), component_t(0, 1), std::move(translation));
    }

    /// A translation matrix.
    /// @param x    X component of the translation vector.
    /// @param y    Y component of the translation vector.
    static Matrix3 get_translation(const element_t x, const element_t y) {
        return Matrix3::get_translation(component_t(x, y));
    }

    /// A rotation matrix.
    /// @param radians   Counter-clockwise rotation in radians.
    static Matrix3 rotation(const element_t radians) {
        const element_t sine = sin(radians);
        const element_t cosine = cos(radians);
        return Matrix3(cosine, sine, -sine, cosine, 0, 0);
    }

    /// A uniform scale matrix.
    /// @param factor   Uniform scale factor.
    static Matrix3 scaling(const element_t factor) { return Matrix3(factor, 0, 0, factor, 0, 0); }

    /// A non-uniform scale matrix.
    /// You can also achieve reflection by passing (-1, 1) for a reflection over the vertical axis, (1, -1) for over the
    /// horizontal axis or (-1, -1) for a point-reflection with respect to the origin.
    /// @param vec  Non-uniform scale vector.
    static Matrix3 scaling(const component_t& vec) { return Matrix3(vec[0], 0, 0, vec[1], 0, 0); }

    /// A non-uniform scale matrix.
    /// @param x    X component of the scale vector.
    /// @param y    Y component of the scale vector.
    static Matrix3 scaling(const element_t x, const element_t y) { return Matrix3::scaling(component_t(x, y)); }

    /// A non-uniform skew matrix.
    /// @param vec  Non-uniform skew vector.
    static Matrix3 skew(const component_t& vec) { return Matrix3(1, tan(vec[1]), tan(vec[0]), 1, 0, 0); }

    /// A non-uniform skew matrix.
    /// @param x    X component of the skew vector.
    /// @param y    Y component of the skew vector.
    static Matrix3 skew(const element_t x, const element_t y) { return Matrix3::skew(component_t(x, y)); }

    /// Name of this Matrix3 type.
    static constexpr const char* get_name() {
        if constexpr (std::is_same_v<Element, float>) {
            return "M3f";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "M3d";
        }
    }

    /// The translation part of this Xform.
    const component_t& get_translation() const { return data[2]; }

    /// Returns the rotational part of this transformation.
    /// Only works if this is actually a pure rotation matrix!
    /// Use `is_rotation` to test, if in doubt.
    /// @return  Applied rotation in radians.
    element_t get_rotation() const {
        try {
            return atan2(data[0][1], data[1][1]);
        }
        catch (std::domain_error&) {
            return 0; // in case both values are zero
        }
    }

    /// Checks whether the matrix is a pure rotation matrix.
    bool is_rotation() const { return (1 - determinant()) < precision_high<element_t>(); }

    /// Scale factor along the x-axis.
    element_t scale_x() const { return sqrt(data[0][0] * data[0][0] + data[0][1] * data[0][1]); }

    /// Scale factor along the y-axis.
    element_t scale_y() const { return sqrt(data[1][0] * data[1][0] + data[1][1] * data[1][1]); }

    /// Calculates the determinant of the transformation matrix.
    element_t determinant() const { return (data[0][0] * data[1][1]) - (data[1][0] * data[0][1]); }

    /// Concatenation of two transformation matrices.
    /// @param other    Transformation to concatenate.
    Matrix3 operator*(const Matrix3& other) const {
        Matrix3 result;
        result[0][0] = data[0][0] * other[0][0] + data[1][0] * other[0][1],
        result[0][1] = data[0][1] * other[0][0] + data[1][1] * other[0][1],
        result[1][0] = data[0][0] * other[1][0] + data[1][0] * other[1][1],
        result[1][1] = data[0][1] * other[1][0] + data[1][1] * other[1][1],
        result[2][0] = data[0][0] * other[2][0] + data[1][0] * other[2][1] + data[2][0],
        result[2][1] = data[0][1] * other[2][0] + data[1][1] * other[2][1] + data[2][1];
        return result;
    }

    /// Concatenation of another transformation matrix to this one in-place.
    /// @param other    Transformation to concatenate.
    Matrix3& operator*=(const Matrix3& other) {
        *this = *this * other;
        return *this;
    }

    /// Concatenate this to another another transformation matrix.
    /// @param other    Transformation to concatenate to.
    Matrix3 premult(const Matrix3& other) const { return other * *this; }

    /// Translates this transformation by a given delta vector.
    /// @param delta    Delta translation.
    Matrix3 translate(const component_t& delta) const { return Matrix3(data[0], data[1], data[2] + delta); }

    /// Rotates the transformation by a given angle in radians.
    /// @param radians  Rotation in radians.
    Matrix3 rotate(const element_t radians) const { return Matrix3::rotation(radians) * *this; }

    /// Returns the inverse of this matrix.
    Matrix3 inverse() const {
        const element_t det = determinant();
        if (abs(det) <= precision_high<element_t>()) { return Matrix3::identity(); }
        const element_t invdet = 1 / det;

        Matrix3 result;
        result[0][0] = +(data[1][1]) * invdet;
        result[1][0] = -(data[1][0]) * invdet;
        result[2][0] = +(data[1][0] * data[2][1] - data[2][0] * data[1][1]) * invdet;
        result[0][1] = -(data[0][1]) * invdet;
        result[1][1] = +(data[0][0]) * invdet;
        result[2][1] = -(data[0][0] * data[2][1] - data[2][0] * data[0][1]) * invdet;
        return result;
    }

    /// Returns the transformed copy of a given vector.
    /// @param vector   Vector to transform.
    component_t transform(const component_t& vector) const {
        return {
            data[0][0] * vector[0] + data[1][0] * vector[1] + data[2][0],
            data[0][1] * vector[0] + data[1][1] * vector[1] + data[2][1],
        };
    }

    //    /// Return the transformed copy of the value.
    //    /// @param value    Value to transform.
    //    template<typename T>
    //    T transform(const T& value) const
    //    {
    //        return detail::matrix3_transform(*this, value);
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
std::ostream& operator<<(std::ostream& out, const notf::detail::Matrix3<Element>& mat) {
    return out << fmt::format("{}", mat);
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Matrix3<Element>> {
    using type = notf::detail::Matrix3<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& mat, FormatContext& ctx) {
        return format_to(ctx.begin(),
                         "{}({: #7.6g}, {: #7.6g}, {: #7.6g}\n"
                         "    {: #7.6g}, {: #7.6g}, {: #7.6g}\n"
                         "    {:8}, {:8}, {:8})",
                         type::get_name(), //
                         mat[0][0], mat[1][0], mat[2][0], mat[0][1], mat[1][1], mat[2][1], 0, 0, 1);
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for Matrix3.
template<class Element>
struct hash<notf::detail::Matrix3<Element>> {
    size_t operator()(const notf::detail::Matrix3<Element>& matrix) const {
        return notf::hash(notf::to_number(notf::detail::HashID::MATRIX3), matrix[0], matrix[1], matrix[2]);
    }
};

} // namespace std

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::M3f>);
static_assert(std::is_pod_v<notf::M3d>);

static_assert(sizeof(notf::M3f) == sizeof(float) * 6);
static_assert(sizeof(notf::M3d) == sizeof(double) * 6);
