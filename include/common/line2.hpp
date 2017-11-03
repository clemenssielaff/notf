#pragma once

#include <iosfwd>

#include "common/aabr.hpp"
#include "common/vector2.hpp"

namespace notf {

struct Line2 {

    /** Start point of the Line2. */
    Vector2f _start;

    /** Vector from the start of the Line2 to its end point. */
    Vector2f _delta;

    /* Do not implement the default methods, so this data structure remains a POD. */
    Line2()                   = default;            // Default Constructor.
    ~Line2()                  = default;            // Destructor.
    Line2(const Line2& other) = default;            // Copy Constructor.
    Line2& operator=(const Line2& other) = default; // Assignment Operator.

    /** Creates a Line from a start- and an end-point. */
    static Line2 from_points(const Vector2f start, const Vector2f& end)
    {
        const Vector2f delta = end - start;
        return {std::move(start), std::move(delta)};
    }

    /** Start point of the Line2. */
    const Vector2f& start() const { return _start; }

    /** Difference between the end and start point of the Line2. */
    const Vector2f& delta() const { return _delta; }

    /** End point of the Line2. */
    Vector2f end() const { return _start + _delta; }

    /** The length of this Line2. */
    float length() const { return _delta.magnitude(); }

    /** The squared length of this Line2 (is faster than `lenght()`). */
    float length_sq() const { return _delta.magnitude_sq(); }

    /** Has the Line2 zero length? */
    bool is_zero() const { return _delta.is_zero(); }

    /** The AABR of this Line2. */
    Aabrf bounding_rect() const { return {_start, std::move(end())}; }

    /** Sets a new start point for this Line2.
     * Updates the complete Line2 - if you have a choice, favor setting the end point rather than the start point.
     */
    Line2& set_start(const Vector2f start)
    {
        const Vector2f _end = end();

        _start = std::move(start);
        _delta = _end - _start;
        return *this;
    }

    /** Sets a new end point for this Line2. */
    Line2& set_end(const Vector2f& end)
    {
        _delta = end - _start;
        return *this;
    }

    /** The x-coordinate where this Line2 crosses a given y-coordinate as it extendeds into infinity.
     * If the Line2 is parallel to the x-axis, the returned coordinate is NAN.
     */
    float x_at(float y) const
    {
        if (_delta.y() == approx(0)) {
            return NAN;
        }
        const float factor = (y - _start.y()) / _delta.y();
        return _start.x() + (_delta.x() * factor);
    }

    /** The y-coordinate where this Line2 crosses a given x-coordinate as it extendeds into infinity.
     * If the Line2 is parallel to the y-axis, the returned coordinate is NAN.
     */
    float y_at(float x) const
    {
        if (_delta.x() == approx(0)) {
            return NAN;
        }
        const float factor = (x - _start.x()) / _delta.x();
        return _start.y() + (_delta.y() * factor);
    }

    /** The slope of this Line2.
     * If the Line is vertical, the slope is infinity.
     */
    float slope() const { return _delta.slope(); }

    /** Checks whether this Line2 is parallel to another. */
    bool is_parallel_to(const Line2& other) const { return _delta.is_parallel_to(other._delta); }

    /** Checks whether this Line2 is orthogonal to another. */
    bool is_orthogonal_to(const Line2& other) const { return _delta.is_orthogonal_to(other._delta); }

    /** Returns the point on this Line2 that is closest to a given position.
     * If the line has a length of zero, the start point is returned.
     * @param point     Position to find the closest point on this Line2 to.
     * @param inside    If true, the closest point has to lie within this Line2's segment (between start and end).
     */
    Vector2f closest_point(const Vector2f& point, bool inside = true) const;

    /** Calculates the intersection of this line with another ... IF they intersect.
     * @param intersection  Out argument, is filled wil the intersection point if an intersection was found.
     * @param other         Line2 to find an intersection with.
     * @param in_self       True, if the intersection point has to lie within the bounds of this Line2.
     * @param in_other      True, if the intersection point has to lie within the bounds of the other Line2.
     * @return  False if the lines do not intersect.
     *          If true, the intersection point ist written into the `intersection` argument.
     */
    bool
    intersect(Vector2f& intersection, const Line2& other, const bool in_self = true, const bool in_other = true) const;
};

/* FREE FUNCTIONS *****************************************************************************************************/

/** Prints the contents of this Line2 into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param vec   Vector2 to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Line2& vec);

} // namespace notf

namespace std {

/** std::hash specialization for notf::Line2. */
template<>
struct hash<notf::Line2> {
    size_t operator()(const notf::Line2& line) const { return notf::hash(line.start(), line.end()); }
};
} // namespace std
