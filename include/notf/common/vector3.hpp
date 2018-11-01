#pragma once

#include "notf/meta/stringtype.hpp"

#include "notf/common/arithmetic_vector.hpp"

NOTF_OPEN_NAMESPACE

// vector3 ========================================================================================================== //

namespace detail {

///  3-dimensional mathematical vector containing real numbers.
template<class Element>
struct Vector3 : public ArithmeticVector<Vector3<Element>, Element, 3> {

    // helper ---------------------------------------------------------------------------------- //
private:
    static constexpr auto _get_name()
    {
        if constexpr (std::is_same_v<Element, float>) {
            return "V3f"_id;
        } else if constexpr (std::is_same_v<Element, double>) {
            return "V3d"_id;
        } else if constexpr (std::is_same_v<Element, int>) {
            return "V3i"_id;
        } else if constexpr (std::is_same_v<Element, short>) {
            return "V3s"_id;
        }
    }

    // types --------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = ArithmeticVector<Vector3<Element>, Element, 3>;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Human readable name of this type, used for string formatting.
    using name = decltype(_get_name());

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    Vector3() = default;

    /// Forwarding constructor.
    template<class... Args>
    Vector3(Args&&... args) : super_t(std::forward<Args>(args)...)
    {}

    /// Unit vector along the X-axis.
    static Vector3 x_axis() { return Vector3(1, 0, 0); }

    /// Unit vector along the Y-axis.
    static Vector3 y_axis() { return Vector3(0, 1, 0); }

    /// Unit vector along the Z-axis.
    static Vector3 z_axis() { return Vector3(0, 0, 1); }

    /// Access to the first element in the vector.
    const element_t& x() const { return data[0]; }
    element_t& x() { return data[0]; }

    /// Access to the second element in the vector.
    element_t& y() { return data[1]; }
    const element_t& y() const { return data[1]; }

    /// Access to the third element in the vector.
    element_t& z() { return data[2]; }
    const element_t& z() const { return data[2]; }

    /// Swizzles.
    constexpr Vector3 xyz() const noexcept { return {data[0], data[1], data[2]}; }
    constexpr Vector3 xzy() const noexcept { return {data[0], data[2], data[1]}; }
    constexpr Vector3 yxz() const noexcept { return {data[1], data[0], data[2]}; }
    constexpr Vector3 yzx() const noexcept { return {data[1], data[2], data[0]}; }
    constexpr Vector3 zxy() const noexcept { return {data[2], data[0], data[1]}; }
    constexpr Vector3 zyx() const noexcept { return {data[2], data[1], data[0]}; }

    /// Returns True, if other and self are approximately the same vector.
    /// Vectorsuse distance approximation instead of component-wise approximation.
    /// @param other     Vector to test against.
    /// @param epsilon   Maximal allowed squared distance between the two vectors.
    constexpr bool is_approx(const Vector3& other, const element_t epsilon = precision_high<element_t>()) const
    {
        return (*this - other).magnitude_sq() <= epsilon;
    }

    /// Checks whether this vector is parallel to other.
    /// @note The zero vector is parallel to everything.
    /// @param other    Vector to test against.
    constexpr bool is_parallel_to(const Vector3& other) const
    {
        return cross(other).magnitude_sq() <= precision_low<element_t>();
    }

    /// Checks whether this vector is orthogonal to other.
    /// @note The zero vector is orthogonal to everything.
    /// @param other    Vector to test against.
    bool is_orthogonal_to(const Vector3& other) const { return direction_to(other) < precision_low<element_t>(); }

    /// Calculates the smallest angle between two vectors.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    /// @return Angle in positive radians.
    element_t get_angle_to(const Vector3& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<element_t>()) {
            return acos(dot(other)); // both are unit
        }
        return acos(dot(other) / sqrt(mag_sq_product));
    }

    /// Tests if the other vector is collinear (1), orthogonal(0), opposite (-1) or something in between.
    /// Similar to `angle`, but saving a call to `acos`.
    /// @note Returns zero, if one or both of the input vectors are of zero magnitude.
    /// @param other     Vector to test against.
    element_t get_direction_to(const Vector3& other) const
    {
        const element_t mag_sq_product = super_t::magnitude_sq() * other.magnitude_sq();
        if (mag_sq_product <= precision_high<element_t>()) {
            return 0.; // one or both are zero
        }
        if (mag_sq_product - 1 <= precision_high<element_t>()) {
            return clamp(dot(other), -1., 1.); // both are unit
        }
        return clamp(this->dot(other) / sqrt(mag_sq_product), -1., 1.);
    }

    /// Vector cross product.
    /// The cross product is a vector perpendicular to this one and other.
    /// The magnitude of the cross vector is twice the area of the triangle defined by the two input Vector3s.
    /// @param other     Other vector.
    Vector3 cross(const Vector3& other) const
    {
        return Vector3(y() * other.z() - z() * other.y(),  //
                       z() * other.x() - x() * other.z(),  //
                       x() * other.y() - y() * other.x()); //
    }

    /// Creates a projection of this vector onto an infinite line whose direction is specified by other.
    /// @param other     Vector to project on. If it is not normalized, the projection is scaled alongside with it.
    Vector3 project_on(const Vector3& other) const { return other * dot(other); }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

using V3f = detail::Vector3<float>;
using V3d = detail::Vector3<double>;
using V3i = detail::Vector3<int>;
using V3s = detail::Vector3<short>;

// ================================================================================================================== //

///// Spherical linear interpolation between two vectors.
///// Travels the torque-minimal path at a constant velocity.
///// Taken from http://bulletphysics.org/Bullet/BulletFull/neon_2vec__aos_8h_source.html .
///// @param from      Left Vector, active at fade <= 0.
///// @param to        Right Vector, active at fade >= 1.
///// @param blend     Blend value, clamped to [0, 1].
///// @return          Interpolated vector.
// template<typename element_t>
// detail::Vector3<element_t>
// slerp(const detail::Vector3<element_t>& from, const detail::Vector3<element_t>& to, element_t blend)
//{
//    blend = clamp(blend, 0., 1.);

//    const element_t cos_angle = from.dot(to);
//    element_t scale_0, scale_1;
//    // use linear interpolation if the angle is too small
//    if (cos_angle >= 1 - precision_low<element_t>()) {
//        scale_0 = (1.0f - blend);
//        scale_1 = blend;
//    }
//    // otherwise use spherical interpolation
//    else {
//        const element_t angle = acos(cos_angle);
//        const element_t recip_sin_angle = 1 / sin(angle);

//        scale_0 = (sin(((1.0f - blend) * angle)) * recip_sin_angle);
//        scale_1 = (sin((blend * angle)) * recip_sin_angle);
//    }
//    return (from * scale_0) + (to * scale_1);
//}

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for Vector3.
template<class Element>
struct hash<notf::detail::Vector3<Element>> {
    size_t operator()(const notf::detail::Vector3<Element>& vector) const
    {
        return notf::hash(notf::to_number(notf::detail::HashID::VECTOR3), vector.hash());
    }
};

} // namespace std

// formatting ======================================================================================================= //

namespace fmt {

template<class Element>
struct formatter<notf::detail::Vector3<Element>> {
    using type = notf::detail::Vector3<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& vec, FormatContext& ctx)
    {
        return format_to(ctx.begin(), "{}({}, {}, {})", type::name::c_str(), vec.x(), vec.y(), vec.z());
    }
};

} // namespace fmt

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
template<class Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Vector3<Element>& vec)
{
    return out << fmt::format("{}", vec);
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::V3f>);
static_assert(std::is_pod_v<notf::V3d>);
static_assert(std::is_pod_v<notf::V3i>);
static_assert(std::is_pod_v<notf::V3s>);

static_assert(sizeof(notf::V3f) == sizeof(float) * 3);
static_assert(sizeof(notf::V3d) == sizeof(double) * 3);
static_assert(sizeof(notf::V3i) == sizeof(int) * 3);
static_assert(sizeof(notf::V3s) == sizeof(short) * 3);
