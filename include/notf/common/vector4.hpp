#pragma once

#include "notf/common/arithmetic_vector.hpp"

NOTF_OPEN_NAMESPACE

// vector4 ========================================================================================================== //

namespace detail {

/// 4-dimensional mathematical vector containing real numbers.
template<typename Element>
struct Vector4 : public ArithmeticVector<Vector4<Element>, Element, 4> {

    // types --------------------------------------------------------------------------------- //
public:
    /// Base class.
    using super_t = ArithmeticVector<Vector4<Element>, Element, 4>;

    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Vector4() noexcept = default;

    /// Forwarding constructor.
    template<class... Args>
    constexpr Vector4(Args&&... args) : super_t(std::forward<Args>(args)...) {}

    /// Unit Vector4 along the X-axis.
    static constexpr Vector4 x_axis() { return Vector4(1, 0, 0); }

    /// Unit Vector4 along the Y-axis.
    static constexpr Vector4 y_axis() { return Vector4(0, 1, 0); }

    /// Unit Vector4 along the Z-axis.
    static constexpr Vector4 z_axis() { return Vector4(0, 0, 1); }

    /// Name of this Vector4 type.
    static constexpr const char* get_name() {
        if constexpr (std::is_same_v<Element, float>) {
            return "V4f";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "V4d";
        } else if constexpr (std::is_same_v<Element, int>) {
            return "V4i";
        } else if constexpr (std::is_same_v<Element, short>) {
            return "V4s";
        }
    }

    /// Access to the first element in the vector.
    constexpr element_t& x() { return data[0]; }
    constexpr const element_t& x() const noexcept { return data[0]; }

    /// Access to the second element in the vector.
    constexpr element_t& y() { return data[1]; }
    constexpr const element_t& y() const noexcept { return data[1]; }

    /// Access to the third element in the vector.
    constexpr element_t& z() { return data[2]; }
    constexpr const element_t& z() const noexcept { return data[2]; }

    /// RAccess to the fourth element in the vector.
    constexpr element_t& w() { return data[3]; }
    constexpr const element_t& w() const noexcept { return data[3]; }

    /// Szizzles.
    constexpr Vector4 xyzw() const noexcept { return {data[0], data[1], data[2], data[3]}; }
    constexpr Vector4 xywz() const noexcept { return {data[0], data[1], data[3], data[2]}; }
    constexpr Vector4 xzyw() const noexcept { return {data[0], data[2], data[1], data[3]}; }
    constexpr Vector4 xzwy() const noexcept { return {data[0], data[2], data[3], data[1]}; }
    constexpr Vector4 xwyz() const noexcept { return {data[0], data[3], data[1], data[2]}; }
    constexpr Vector4 xwzy() const noexcept { return {data[0], data[3], data[2], data[1]}; }
    constexpr Vector4 yxzw() const noexcept { return {data[1], data[0], data[2], data[3]}; }
    constexpr Vector4 yxwz() const noexcept { return {data[1], data[0], data[3], data[2]}; }
    constexpr Vector4 yzxw() const noexcept { return {data[1], data[2], data[0], data[3]}; }
    constexpr Vector4 yzwx() const noexcept { return {data[1], data[2], data[3], data[0]}; }
    constexpr Vector4 ywxz() const noexcept { return {data[1], data[3], data[0], data[2]}; }
    constexpr Vector4 ywzx() const noexcept { return {data[1], data[3], data[2], data[0]}; }
    constexpr Vector4 zxyw() const noexcept { return {data[2], data[0], data[1], data[3]}; }
    constexpr Vector4 zxwy() const noexcept { return {data[2], data[0], data[3], data[1]}; }
    constexpr Vector4 zyxw() const noexcept { return {data[2], data[1], data[0], data[3]}; }
    constexpr Vector4 zywx() const noexcept { return {data[2], data[1], data[3], data[0]}; }
    constexpr Vector4 zwxy() const noexcept { return {data[2], data[3], data[0], data[1]}; }
    constexpr Vector4 zwyx() const noexcept { return {data[2], data[3], data[1], data[0]}; }
    constexpr Vector4 wxyz() const noexcept { return {data[3], data[0], data[1], data[2]}; }
    constexpr Vector4 wxzy() const noexcept { return {data[3], data[0], data[2], data[1]}; }
    constexpr Vector4 wyxz() const noexcept { return {data[3], data[1], data[0], data[2]}; }
    constexpr Vector4 wyzx() const noexcept { return {data[3], data[1], data[2], data[0]}; }
    constexpr Vector4 wzxy() const noexcept { return {data[3], data[2], data[0], data[1]}; }
    constexpr Vector4 wzyx() const noexcept { return {data[3], data[2], data[1], data[0]}; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data;
};

} // namespace detail

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash implementation for Vector4.
template<class Element>
struct hash<notf::detail::Vector4<Element>> {
    size_t operator()(const notf::detail::Vector4<Element>& vector) const {
        return notf::hash(notf::to_number(notf::detail::HashID::VECTOR4), vector.get_hash());
    }
};

} // namespace std

// formatting ======================================================================================================= //

namespace fmt {

template<class Element>
struct formatter<notf::detail::Vector4<Element>> {
    using type = notf::detail::Vector4<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& vec, FormatContext& ctx) {
        return format_to(ctx.begin(), "{}({}, {}, {}, {})", type::name::c_str(), vec.x(), vec.y(), vec.z(), vec.w());
    }
};

} // namespace fmt

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
template<class Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Vector4<Element>& vec) {
    return out << fmt::format("{}", vec);
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::V4f>);
static_assert(std::is_pod_v<notf::V4d>);
static_assert(std::is_pod_v<notf::V4i>);
static_assert(std::is_pod_v<notf::V4s>);

static_assert(sizeof(notf::V4f) == sizeof(float) * 4);
static_assert(sizeof(notf::V4d) == sizeof(double) * 4);
static_assert(sizeof(notf::V4i) == sizeof(int) * 4);
static_assert(sizeof(notf::V4s) == sizeof(short) * 4);
