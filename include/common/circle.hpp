#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/vector2.hpp"
#include "common/template.hpp"

namespace notf {

//*********************************************************************************************************************/

/**
 * Simple 2D circle shape.
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _Circle {

    /* Types **********************************************************************************************************/

    using value_t = Real;

    using vector_t = _RealVector2<Real>;

    /* Fields *********************************************************************************************************/

    /** Center position of the Circle. */
    _RealVector2<Real> center;

    /** Radius of the Circle. */
    value_t radius;

    /* Constructors ***************************************************************************************************/

    _Circle() = default; // so this data structure remains a POD

    _Circle(vector_t center, const value_t radius)
        : center(std::move(center))
        , radius(radius)
    {
    }

    _Circle(const value_t radius)
        : center(vector_t::zero())
        , radius(radius)
    {
    }

    /* Static Constructors ********************************************************************************************/

    /** Produces a zero Circle. */
    static _Circle zero() { return {{0, 0}, 0}; }

    /*  Inspection  ***************************************************************************************************/

    /** Checks if this is a zero Circle. */
    bool is_zero() const { return radius == 0.f; }

    /** The diameter of the Circle. */
    value_t diameter() const { return radius * 2.f; }

    /** The circumenfence of this Circle. */
    value_t circumfence() const { return PI * radius * 2.f; }

    /** The area of this Circle. */
    value_t area() const { return PI * radius * radius; }

    /** Checks, if the given point is contained within (or on the border of) this Circle. */
    bool contains(const vector_t& point) const
    {
        return (point - center).get_magnitude_sq() <= (radius * radius);
    }

    /** Checks if the other Circle intersects with this one, intersection requires the intersected area to be >= zero. */
    bool intersects(const _Circle& other) const
    {
        const value_t radii = radius + other.radius;
        return (other.center - center).get_magnitude_sq() < (radii * radii);
    }

    /** Returns the closest point inside this Circle to the given target point. */
    vector_t closest_point_to(const vector_t& target) const
    {
        const vector_t delta = target - center;
        const value_t mag_sq = delta.get_magnitude_sq();
        if (mag_sq <= (radius * radius)) {
            return target;
        }
        if (mag_sq == 0.f) {
            return center;
        }
        return (delta / sqrt(mag_sq)) * radius;
    }

    /** Operators *****************************************************************************************************/

    bool operator==(const _Circle& other) const { return (other.center == center && other.radius == radius); }

    bool operator!=(const _Circle& other) const { return (other.center != center || other.radius != radius); }

    /*  Modifiers *****************************************************************************************************/

    /** Sets this Circle to zero. */
    void set_zero()
    {
        center.set_zero();
        radius = 0.f;
    }
};

//*********************************************************************************************************************/

using Circlef = _Circle<float>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Circle into a std::ostream.
 * @param out       Output stream, implicitly passed with the << operator.
 * @param circle    Circle to print.
 * @return          Output stream for further output.
 */
template <typename Real>
std::ostream& operator<<(std::ostream& out, const notf::_Circle<Real>& circle);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Circle. */
template <typename Real>
struct hash<notf::_Circle<Real>> {
    size_t operator()(const notf::_Circle<Real>& circle) const { return notf::hash(circle.center, circle.radius); }
};
}
