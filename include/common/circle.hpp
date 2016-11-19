#pragma once

#include <iosfwd>

#include "common/float_utils.hpp"
#include "common/hash_utils.hpp"
#include "common/vector2.hpp"

namespace notf {

/**
 * Simple 2D circle shape.
 */
struct Circle {

    /** Center position of the Circle. */
    Vector2 center;

    /** Radius of the Circle. */
    float radius;

    Circle() = default; // so this data structure remains a POD

    Circle(const Vector2 center, const float radius)
        : center(std::move(center))
        , radius(radius)
    {
    }

    explicit Circle(const float radius)
        : center(Vector2::zero())
        , radius(radius)
    {
    }

    /** Produces a null Circle. */
    static Circle null() { return {{0, 0}, 0}; }

    /*  Inspection  ***************************************************************************************************/

    /** Checks if this is a null Circle. */
    bool is_null() const { return radius == 0.f; }

    /** The diameter of the Circle. */
    float diameter() const { return radius * 2.f; }

    /** The circumenfence of this Circle. */
    float circumfence() const { return PI * radius * 2.f; }

    /** The area of this Circle. */
    float area() const { return PI * radius * radius; }

    /** Checks, if the given point is contained within (or on the border of) this Circle. */
    bool contains(const Vector2& point) const
    {
        return (point - center).magnitude_sq() <= (radius * radius);
    }

    /** Checks if the other Circle intersects with this one, intersection requires the intersected area to be >= zero. */
    bool intersects(const Circle& other) const
    {
        const float radii = radius + other.radius;
        return (other.center - center).magnitude_sq() < (radii * radii);
    }

    /** Returns the closest point inside this Circle to the given target point. */
    Vector2 closest_point_to(const Vector2& target) const
    {
        const Vector2 delta = target - center;
        const float mag_sq = delta.magnitude_sq();
        if (mag_sq <= (radius * radius)) {
            return target;
        }
        if (mag_sq == 0.f) {
            return center;
        }
        return (delta / sqrt(mag_sq)) * radius;
    }

    /** Operators *****************************************************************************************************/

    bool operator==(const Circle& other) const { return (other.center == center && other.radius == radius); }

    bool operator!=(const Circle& other) const { return (other.center != center || other.radius != radius); }

    /*  Modification **************************************************************************************************/

    /** Sets this Circle to null. */
    void set_null()
    {
        center.set_null();
        radius = 0.f;
    }
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Circle into a std::ostream.
 * @param out       Output stream, implicitly passed with the << operator.
 * @param circle    Circle to print.
 * @return          Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Circle& circle);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Circle. */
template <>
struct hash<notf::Circle> {
    size_t operator()(const notf::Circle& circle) const { return notf::hash(circle.center, circle.radius); }
};
}
