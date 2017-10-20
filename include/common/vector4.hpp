#pragma once

#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//====================================================================================================================//

/// @brief 4-dimensional mathematical vector containing real numbers.
template <typename REAL, ENABLE_IF_REAL(REAL)>
struct _RealVector4 : public detail::Arithmetic<_RealVector4<REAL>, REAL, 4> {

    /// @brief Element type.
    using element_t = REAL;

    /// @brief Arithmetic base type.
    using super_t = detail::Arithmetic<_RealVector4<REAL>, REAL, 4>;

    // members -------------------------------------------------------------------------------------------------------//
    using super_t::data;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    _RealVector4() = default;

    /// @brief Element-wise constructor.
    /// @param x    First component.
    /// @param y    Second component (default is 0).
    /// @param z    Third component (default is 0).
    /// @param w    Fourth component (default is 1).
    _RealVector4(const element_t x, const element_t y = 0, const element_t z = 0, const element_t w = 1)
        : super_t{x, y, z, w} {}

    /// @brief Unit Vector2 along the X-axis.
    static _RealVector4 x_axis() { return _RealVector4(1, 0, 0); }

    /// @brief Unit Vector2 along the Y-axis.
    static _RealVector4 y_axis() { return _RealVector4(0, 1, 0); }

    /// @brief Unit Vector2 along the Z-axis.
    static _RealVector4 z_axis() { return _RealVector4(0, 0, 1); }

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
};

//====================================================================================================================//

using Vector4f = _RealVector4<float>;
using Vector4d = _RealVector4<double>;

//====================================================================================================================//

/// @brief Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Vector4f& vec);
std::ostream& operator<<(std::ostream& out, const Vector4d& vec);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash implementation for notf::_RealVector4.
template <typename Real>
struct hash<notf::_RealVector4<Real>> {
    size_t operator()(const notf::_RealVector4<Real>& vector) const { return vector.hash(); }
};

} // namespace std

//#ifndef NOTF_NO_SIMD
//#include "common/simd/simd_arithmetic4f.hpp"
//#include "common/simd/simd_vector4f.hpp"
//#endif
