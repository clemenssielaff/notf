#pragma once

#include <iosfwd>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/vector2.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/**
 * Simple 2D circle shape.
 */
template <typename Real, ENABLE_IF_REAL(Real)>
struct _Circle {

    /* Types **********************************************************************************************************/

    using Value_t = Real;

    using Vector_t = _RealVector2<Real>;

    /* Fields *********************************************************************************************************/

    /** Center position of the Circle. */
    _RealVector2<Real> center;

    /** Radius of the Circle. */
    Value_t radius;

    /* Constructors ***************************************************************************************************/

    _Circle() = default; // so this data structure remains a POD

    _Circle(Vector_t center, const Value_t radius)
        : center(std::move(center))
        , radius(radius)
    {
    }

    _Circle(const Value_t radius)
        : center(Vector_t::zero())
        , radius(radius)
    {
    }

    /* Static Constructors ********************************************************************************************/

    /** Produces a null Circle. */
    static _Circle null() { return {{0, 0}, 0}; }

    /*  Inspection  ***************************************************************************************************/

    /** Checks if this is a null Circle. */
    bool is_null() const { return radius == 0.f; }

    /** The diameter of the Circle. */
    Value_t diameter() const { return radius * 2.f; }

    /** The circumenfence of this Circle. */
    Value_t circumfence() const { return PI * radius * 2.f; }

    /** The area of this Circle. */
    Value_t area() const { return PI * radius * radius; }

    /** Checks, if the given point is contained within (or on the border of) this Circle. */
    bool contains(const Vector_t& point) const
    {
        return (point - center).magnitude_sq() <= (radius * radius);
    }

    /** Checks if the other Circle intersects with this one, intersection requires the intersected area to be >= zero. */
    bool intersects(const _Circle& other) const
    {
        const Value_t radii = radius + other.radius;
        return (other.center - center).magnitude_sq() < (radii * radii);
    }

    /** Returns the closest point inside this Circle to the given target point. */
    Vector_t closest_point_to(const Vector_t& target) const
    {
        const Vector_t delta = target - center;
        const Value_t mag_sq = delta.magnitude_sq();
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

    /** Sets this Circle to null. */
    void set_null()
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
