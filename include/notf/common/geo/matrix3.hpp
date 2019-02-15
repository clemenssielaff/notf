#pragma once

#include "notf/common/geo/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// matrix3 ========================================================================================================== //

/// 2D Transformation matrix.
/// Stores elements colum-major like OpenGL (see https://stackoverflow.com/a/17718408):
/// visually:   |a c x|
///             |b d y|
///             |0 0 1| // last row is implicit
/// in memory: [a, b, c, d, x, y], where (x, y) is the translation vector.
template<class Element>
struct Matrix3 : public Arithmetic<Matrix3<Element>, Vector2<Element>, 3> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class.
    using super_t = Arithmetic<Matrix3<Element>, Vector2<Element>, 3>;

public:
    /// Component type used by this arithmetic type.
    /// In a vector, this is the same as element_t, whereas in a matrix it will be a vector.
    using component_t = typename super_t::component_t;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Data holder.
    using Data = typename super_t::Data;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Matrix3() noexcept = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr Matrix3(Args&&... args) noexcept : super_t(std::forward<Args>(args)...) {}

    /// Name of this Matrix3 type.
    static constexpr const char* get_name() {
        if constexpr (std::is_same_v<Element, float>) {
            return "M3f";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "M3d";
        }
    }

    /// @{
    /// Constructs the Matrix with given diagonal elements.
    /// @param a    a element.
    /// @param d    d element.
    static constexpr Matrix3 diagonal(const element_t a, const element_t d) noexcept {
        return Matrix3(component_t{a, 0}, component_t{0, d}, component_t{0, 0});
    }
    static constexpr Matrix3 diagonal(const element_t ad) noexcept {
        return Matrix3(component_t{ad, 0}, component_t{0, ad}, component_t{0, 0});
    }
    /// @}

    /// The identity matrix.
    static constexpr Matrix3 identity() noexcept { return diagonal(1); }

    /// Matrix with each element set to zero.
    static constexpr Matrix3 zero() noexcept { return diagonal(0); }

    /// @{
    /// A translation matrix.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Affine_transformations
    /// @param translation  Translation vector.
    static constexpr Matrix3 translation(component_t translation) noexcept {
        return Matrix3(component_t{1, 0}, component_t{0, 1}, std::move(translation));
    }
    static constexpr Matrix3 translation(const element_t x, const element_t y) noexcept {
        return Matrix3::translation(component_t(x, y));
    }
    /// @}

    /// Matrix representing a counterclockwise 2D rotation around an arbitrary pivot point
    /// This is a concatenation of the following:
    /// 1. Translate the coordinates so that `pivot` is at the origin.
    /// 2. Rotate.
    /// 3. Translate back.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Rotation
    /// @param radian   Angle to rotate counterclockwise in radian.
    /// @param pivot    Pivot point to rotate around.
    static Matrix3 rotation(const element_t radian, const component_t& pivot = component_t::zero()) noexcept {
        const element_t sin = fast_sin(radian);
        const element_t cos = fast_cos(radian);
        return {component_t{cos, sin}, component_t{-sin, cos},
                component_t{pivot[0] - cos * pivot[0] + sin * pivot[1], //
                            pivot[1] - sin * pivot[0] - cos * pivot[1]}};
    }

    /// A uniform scale matrix.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Stretching
    /// @param factor   Uniform scale factor.
    static constexpr Matrix3 scale(const element_t factor) noexcept { return diagonal(factor); }

    /// @{
    /// A non-uniform scale matrix.
    /// You can also achieve reflection by passing (-1, 1) for a reflection over the vertical axis, (1, -1) for over the
    /// horizontal axis or (-1, -1) for a point-reflection with respect to the origin.
    /// @param x    X component of the scale vector.
    /// @param y    Y component of the scale vector.
    static constexpr Matrix3 scale(const element_t x, const element_t y) noexcept { return diagonal(x, y); }
    static constexpr Matrix3 scale(const component_t& vec) noexcept { return scaling(vec[0], vec[1]); }
    /// @}

    /// Squeeze transformation.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Squeezing
    /// @param factor    Squeeze factor.
    static constexpr Matrix3 squeeze(const element_t factor) noexcept {
        if (is_zero(factor, precision_high<element_t>())) {
            return zero();
        } else {
            return diagonal(factor, 1 / factor);
        }
    }

    /// @{
    /// A non-uniform shear matrix.
    /// See https://en.wikipedia.org/wiki/Transformation_matrix#Shearing
    /// @param x    X distance of the shear.
    /// @param y    Y distance of the shear.
    static constexpr Matrix3 shear(const element_t x, const element_t y) noexcept {
        return {component_t{1, y}, component_t{x, 1}, component_t{0, 0}};
    }
    static constexpr Matrix3 shear(const component_t& vec) noexcept { return shear(vec[0], vec[1]); }
    /// @}

    /// @{
    /// Reflection over a line defined by two points.
    /// This is a concatenation of the following:
    /// 1. Translate the coordinates so that `start` is at the origin.
    /// 2. Rotate so that `direction-start` aligns with the x-axis.
    /// 3. Reflect about the x-axis.
    /// 4. Rotate back.
    /// 5. Translate back.
    /// @param start        First point on the reflection line.
    /// @param direction    Second point on the reflection line.
    static Matrix3 reflection(const component_t& start, component_t direction) noexcept {
        direction = direction - start;
        if (is_zero(direction[1], precision_high<element_t>())) { return scale(1, -1); } // line is horizontal
        if (is_zero(direction[0], precision_high<element_t>())) { return scale(-1, 1); } // line is vertical
        {
            const element_t magnitude_sq = direction.get_magnitude_sq();
            if (is_zero(magnitude_sq, precision_high<element_t>())) { return identity(); }
            if (!is_approx(magnitude_sq, 1)) { direction /= std::sqrt(magnitude_sq); }
        }
        const element_t u = direction[0] * direction[0] - direction[1] * direction[1];
        const element_t v = direction[0] * direction[1] + direction[0] * direction[1];
        return {component_t{u, v}, component_t{v, -u},
                component_t{start[0] - u * start[0] - v * start[1], //
                            start[1] + u * start[1] - v * start[0]}};
    }
    static Matrix3 reflection(component_t direction) noexcept {
        return reflection(component_t{0, 0}, std::move(direction));
    }
    /// @}

    /// Reflection over a line that passes through the origin at the given angle in radian.
    /// @param angle    Counterclockwise angle of the reflection line in radian.
    static Matrix3 reflection(const element_t angle) {
        const element_t sin = fast_sin(2 * angle);
        const element_t cos = fast_cos(2 * angle);
        return {component_t{cos, sin}, component_t{sin, -cos}, component_t{0, 0}};
    }

    /// A 2D transformation preserves the area of a polygon if its determinant is +/-1.
    constexpr element_t get_scale_factor() const noexcept {
        return sqrt(data[0].get_magnitude_sq() * data[1].get_magnitude_sq());
    }

    /// Determinant of an affine 2D transformation matrix:
    ///          ||a c x||
    /// det(M) = ||b d y|| => a*d*1 + c*y*0 + x*b*0 - 0*d*x - 0*y*a - 1*b*c => a*d - b*c
    ///          ||0 0 1||
    constexpr element_t get_determinant() const noexcept { return data[0][0] * data[1][1] - data[0][1] * data[1][0]; }

    /// The inverse transformation matrix.
    /// Will fail if the matrix is singular (not invertible).
    ///                 | d -c (cx-dx)|
    /// -M = 1/det(M) * |-b  a (bx-ay)|
    ///                 | 0  0 (ad-bc)|
    /// Note that det(M) == (ad-bc), therefore the bottom row will always be |0 0 1|.
    /// See https://www.wikihow.com/Find-the-Inverse-of-a-3x3-Matrix
    constexpr Matrix3 get_inverse() const noexcept {
        const element_t determinant = get_determinant();
        if (is_zero(determinant, precision_high<element_t>())) { // not invertible
            return identity();
        }
        return Matrix3{component_t{data[1][1], -data[0][1]}, //
                       component_t{-data[1][0], data[0][0]},
                       component_t{
                           (data[1][0] * data[2][1] - data[1][1] * data[2][0]),
                           (data[0][1] * data[2][0] - data[0][0] * data[2][1]),
                       }}
               / determinant;
    }

    /// @{
    /// Concatenation the other matrix transformation to this one and returns the result.
    /// Inputs:
    ///     this = |a c e|, other = |u w y|
    ///            |b d f|          |v x z|
    ///            |0 0 1|          |0 0 1| // last row is implicit
    /// Operation:
    ///           |u w y         h = a*u + c*v + e*0
    ///           |v x z         i = b*u + d*v + f*0
    ///           |0 0 1   with  j = a*w + c*x + e*0
    ///     ------+------        k = b*w + d*x + f*0
    ///     a c e |h j l         l = a*y + c*z + e*1
    ///     b d f |i k m         m = b*y + d*z + f*1
    ///     0 0 1 |0 0 1
    ///
    constexpr Matrix3 operator*(const Matrix3& other) const& noexcept {
        return {component_t{data[0][0] * other[0][0] + data[1][0] * other[0][1],
                            data[0][1] * other[0][0] + data[1][1] * other[0][1]},
                component_t{data[0][0] * other[1][0] + data[1][0] * other[1][1],
                            data[0][1] * other[1][0] + data[1][1] * other[1][1]},
                component_t{data[0][0] * other[2][0] + data[1][0] * other[2][1] + data[2][0],
                            data[0][1] * other[2][0] + data[1][1] * other[2][1] + data[2][1]}};
    }
    constexpr Matrix3 operator*(const Matrix3& other) && noexcept { return *this *= other; }
    /// @}

    /// Concatenate the other matrix transformation to this one in place.
    /// @param other    Transformation to concatenate.
    constexpr Matrix3& operator*=(const Matrix3& other) noexcept {
        *this = *this * other;
        return *this;
    }

    /// Concatenate a translation to this transformation.
    /// @param delta    Translation to add.
    constexpr Matrix3& translate(const component_t& delta) noexcept {
        data[2] += delta;
        return *this;
    }

    /// Concatenate a counterclockwise rotation to this transformation.
    /// @param angle    Rotation to add.
    constexpr Matrix3& rotate(const element_t angle) noexcept {
        this *= Matrix3::rotation(angle);
        return *this;
    }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

// transformations ================================================================================================== //
// clang-format off

// v2 * m3
template<> V2f transform_by<V2f, M3f>(const V2f&, const M3f&);
template<> V2d transform_by<V2d, M3f>(const V2d&, const M3f&);

template<> V2f transform_by<V2f, M3d>(const V2f&, const M3d&);
template<> V2d transform_by<V2d, M3d>(const V2d&, const M3d&);

// aabr * m3
template<> Aabrf transform_by<Aabrf, M3f>(const Aabrf&, const M3f&);
template<> Aabrd transform_by<Aabrd, M3f>(const Aabrd&, const M3f&);

template<> Aabrf transform_by<Aabrf, M3d>(const Aabrf&, const M3d&);
template<> Aabrd transform_by<Aabrd, M3d>(const Aabrd&, const M3d&);

// polygon * m3
template<> Polygonf transform_by<Polygonf, M3f>(const Polygonf&, const M3f&);

// bezier * m3
template<> CubicBezier2f transform_by<CubicBezier2f, M3f>(const CubicBezier2f&, const M3f&);
template<> CubicBezier2d transform_by<CubicBezier2d, M3f>(const CubicBezier2d&, const M3f&);

template<> CubicBezier2f transform_by<CubicBezier2f, M3d>(const CubicBezier2f&, const M3d&);
template<> CubicBezier2d transform_by<CubicBezier2d, M3d>(const CubicBezier2d&, const M3d&);

// clang-format on
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

/// std::hash specialization for Matrix3.
template<class Element>
struct std::hash<notf::detail::Matrix3<Element>> {
    size_t operator()(const notf::detail::Matrix3<Element>& matrix) const {
        return notf::hash(notf::to_number(notf::detail::HashID::MATRIX3), matrix[0], matrix[1], matrix[2]);
    }
};

// compile time tests =============================================================================================== //

static_assert(notf::is_pod_v<notf::M3f>);
static_assert(notf::is_pod_v<notf::M3d>);

static_assert(sizeof(notf::M3f) == sizeof(float) * 6);
static_assert(sizeof(notf::M3d) == sizeof(double) * 6);
