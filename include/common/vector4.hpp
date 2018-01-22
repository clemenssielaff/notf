#pragma once

#include "common/arithmetic.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief 4-dimensional mathematical vector containing real numbers.
template<typename REAL>
struct RealVector4 : public detail::Arithmetic<RealVector4<REAL>, REAL, 4> {

    /// @brief Element type.
    using element_t = REAL;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<RealVector4<REAL>, REAL, 4>;

    // fields --------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    RealVector4() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    /// @param w    Fourth component (default is 1).
    template<typename X, typename Y = element_t, typename Z = element_t, typename W = element_t>
    RealVector4(const X x, const Y y = 0, const Z z = 0, const W w = 1)
        : super_t{static_cast<element_t>(x), static_cast<element_t>(y), static_cast<element_t>(z),
                  static_cast<element_t>(w)}
    {}

    /// @brief Unit Vector2 along the X-axis.
    static RealVector4 x_axis() { return RealVector4(1, 0, 0); }

    /// @brief Unit Vector2 along the Y-axis.
    static RealVector4 y_axis() { return RealVector4(0, 1, 0); }

    /// @brief Unit Vector2 along the Z-axis.
    static RealVector4 z_axis() { return RealVector4(0, 0, 1); }

    /// @brief Read-write access to the first element in the vector.
    element_t& x() { return data[0]; }

    /// @brief Read-write access to the second element in the vector.
    element_t& y() { return data[1]; }

    /// @brief Read-write access to the third element in the vector.
    element_t& z() { return data[2]; }

    /// @brief Read-write access to the fourth element in the vector.
    element_t& w() { return data[3]; }

    /// @brief Read-only access to the first element in the vector.
    const element_t& x() const { return data[0]; }

    /// @brief Read-only access to the second element in the vector.
    const element_t& y() const { return data[1]; }

    /// @brief Read-only access to the third element in the vector.
    const element_t& z() const { return data[2]; }

    /// @brief Read-only access to the fourth element in the vector.
    const element_t& w() const { return data[3]; }

    /// @brief Szizzles.
    RealVector4 xyzw() const { return {data[0], data[1], data[2], data[3]}; }
    RealVector4 xywz() const { return {data[0], data[1], data[3], data[2]}; }
    RealVector4 xzyw() const { return {data[0], data[2], data[1], data[3]}; }
    RealVector4 xzwy() const { return {data[0], data[2], data[3], data[1]}; }
    RealVector4 xwyz() const { return {data[0], data[3], data[1], data[2]}; }
    RealVector4 xwzy() const { return {data[0], data[3], data[2], data[1]}; }
    RealVector4 yxzw() const { return {data[1], data[0], data[2], data[3]}; }
    RealVector4 yxwz() const { return {data[1], data[0], data[3], data[2]}; }
    RealVector4 yzxw() const { return {data[1], data[2], data[0], data[3]}; }
    RealVector4 yzwx() const { return {data[1], data[2], data[3], data[0]}; }
    RealVector4 ywxz() const { return {data[1], data[3], data[0], data[2]}; }
    RealVector4 ywzx() const { return {data[1], data[3], data[2], data[0]}; }
    RealVector4 zxyw() const { return {data[2], data[0], data[1], data[3]}; }
    RealVector4 zxwy() const { return {data[2], data[0], data[3], data[1]}; }
    RealVector4 zyxw() const { return {data[2], data[1], data[0], data[3]}; }
    RealVector4 zywx() const { return {data[2], data[1], data[3], data[0]}; }
    RealVector4 zwxy() const { return {data[2], data[3], data[0], data[1]}; }
    RealVector4 zwyx() const { return {data[2], data[3], data[1], data[0]}; }
    RealVector4 wxyz() const { return {data[3], data[0], data[1], data[2]}; }
    RealVector4 wxzy() const { return {data[3], data[0], data[2], data[1]}; }
    RealVector4 wyxz() const { return {data[3], data[1], data[0], data[2]}; }
    RealVector4 wyzx() const { return {data[3], data[1], data[2], data[0]}; }
    RealVector4 wzxy() const { return {data[3], data[2], data[0], data[1]}; }
    RealVector4 wzyx() const { return {data[3], data[2], data[1], data[0]}; }
};

} // namespace detail

//====================================================================================================================//

using Vector4f = detail::RealVector4<float>;
using Vector4d = detail::RealVector4<double>;
using Vector4h = detail::RealVector4<half>;

//====================================================================================================================//

/// @brief Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector4f& vec);
std::ostream& operator<<(std::ostream& out, const Vector4d& vec);
std::ostream& operator<<(std::ostream& out, const Vector4h& vec);

} // namespace notf

//== std::hash =======================================================================================================//

namespace std {

/// @brief std::hash implementation for RealVector4.
template<typename Real>
struct hash<notf::detail::RealVector4<Real>> {
    size_t operator()(const notf::detail::RealVector4<Real>& vector) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::VECTOR), vector.hash());
    }
};

} // namespace std
