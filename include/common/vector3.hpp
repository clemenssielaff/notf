#pragma once

#include <assert.h>
#include <iosfwd>

#include "common/arithmetic.hpp"
#include "common/meta.hpp"

namespace notf {

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector containing real numbers. */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _RealVector3 : public detail::Arithmetic<_RealVector3<Real>, Real, 3> {

    /* Types **********************************************************************************************************/

    using super   = detail::Arithmetic<_RealVector3<Real>, Real, 3>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _RealVector3() = default;

    /** Element-wise constructor. */
    template <typename A, typename B, typename C>
    _RealVector3(A x, B y, C z)
        : super{static_cast<value_t>(x), static_cast<value_t>(y), static_cast<value_t>(z)} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _RealVector3 x_axis() { return _RealVector3(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _RealVector3 y_axis() { return _RealVector3(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _RealVector3 z_axis() { return _RealVector3(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Returns True, if other and self are approximately the same Vector3.
     * @param other     Vector3 to test against.
     * @param epsilon   Maximal allowed distance between the two Vectors.
     */
    bool is_approx(const _RealVector3& other, const value_t epsilon = precision_high<value_t>()) const
    {
        return (*this - other).get_magnitude_sq() <= epsilon * epsilon;
    }

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Read-only access to the third element in the vector. */
    const value_t& z() const { return data[2]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Read-write access to the third element in the vector. */
    value_t& z() { return data[2]; }
};

//*********************************************************************************************************************/

/** 2-dimensional mathematical Vector3 containing integers. */
template <typename Integer, ENABLE_IF_INT(Integer)>
struct _IntVector3 : public detail::Arithmetic<_IntVector3<Integer>, Integer, 3> {

    // explitic forwards
    using super   = detail::Arithmetic<_IntVector3<Integer>, Integer, 3>;
    using value_t = typename super::value_t;
    using super::data;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _IntVector3() = default;

    /** Perforect forwarding constructor. */
    template <typename... T>
    _IntVector3(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    /* Static Constructors ********************************************************************************************/

    /** Unit Vector3 along the X-axis. */
    static _IntVector3 x_axis() { return _IntVector3(1, 0, 0); }

    /** Unit Vector3 along the Y-axis. */
    static _IntVector3 y_axis() { return _IntVector3(0, 1, 0); }

    /** Unit Vector3 along the Z-axis. */
    static _IntVector3 z_axis() { return _IntVector3(0, 0, 1); }

    /*  Inspection  ***************************************************************************************************/

    /** Read-only access to the first element in the vector. */
    const value_t& x() const { return data[0]; }

    /** Read-only access to the second element in the vector. */
    const value_t& y() const { return data[1]; }

    /** Read-only access to the third element in the vector. */
    const value_t& z() const { return data[2]; }

    /** Modification **************************************************************************************************/

    /** Read-write access to the first element in the vector. */
    value_t& x() { return data[0]; }

    /** Read-write access to the second element in the vector. */
    value_t& y() { return data[1]; }

    /** Read-write access to the second element in the vector. */
    value_t& z() { return data[2]; }
};

//*********************************************************************************************************************/

using Vector3f = _RealVector3<float>;
using Vector3d = _RealVector3<double>;
using Vector3i = _IntVector3<int>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of a Vector3 into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param vec  Vector3 to print.
 * @return Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Vector3f& vec);
std::ostream& operator<<(std::ostream& out, const Vector3d& vec);
std::ostream& operator<<(std::ostream& out, const Vector3i& vec);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_RealVector3. */
template <typename Real>
struct hash<notf::_RealVector3<Real>> {
    size_t operator()(const notf::_RealVector3<Real>& vector) const { return vector.hash(); }
};

/** std::hash specialization for notf::_IntVector3. */
template <typename Integer>
struct hash<notf::_IntVector3<Integer>> {
    size_t operator()(const notf::_IntVector3<Integer>& vector) const { return vector.hash(); }
};

} // namespace std
