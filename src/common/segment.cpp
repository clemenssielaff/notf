#include "common/segment.hpp"

#include <iostream>
#include <type_traits>

NOTF_OPEN_NAMESPACE

// Vector2f Line2::closest_point(const Vector2f& point, bool inside) const
//{
//    const float length_sq = _delta.magnitude_sq();
//    if (length_sq == approx(0.)) {
//        return _start;
//    }
//    const float dot = (point - _start).dot(_delta);
//    float t         = -dot / length_sq;
//    if (inside) {
//        t = clamp(t, 0.0, 1.0);
//    }
//    return _start + (_delta * t);
//}

// bool Line2::intersect(Vector2f& intersection, const Line2& other, const bool in_self, const bool in_other) const
//{
//    // fail if either line segment is zero-length or if the two lines are parallel
//    if (is_zero() || other.is_zero() || is_parallel_to(other)) {
//        return false;
//    }

//    // if this line is vertical, calculate intersection from the other line
//    if (_delta.is_vertical()) {
//        intersection.x() = _start.x();
//        intersection.y() = other.y_at(_start.x());
//        return true;
//    }

//    // if the other line is vertical, do the same thing from this one
//    if (other.delta().is_vertical()) {
//        intersection.x() = other.start().x();
//        intersection.y() = y_at(other.start().x());
//        return true;
//    }

//    // if none of the lines are vertical, calculate the x coordinate
//    float line_slope  = slope();
//    float y_intersect = y_at(0.0);
//    float x_coord     = (y_intersect - other.y_at(0.0)) / (other.slope() - line_slope);

//    // test, if the intersection is within the given boundaries
//    if (in_self) {
//        float min_x, max_x;
//        if (_delta.x() >= 0) {
//            min_x = _start.x();
//            max_x = _start.x() + _delta.x();
//        }
//        else {
//            min_x = _start.x() + _delta.x();
//            max_x = _start.x();
//        }
//        if ((x_coord < min_x) || (x_coord > max_x)) {
//            return false;
//        }
//    }
//    if (in_other) {
//        float min_x, max_x;
//        if (other.delta().x() >= 0) {
//            min_x = other.start().x();
//            max_x = other.start().x() + other.delta().x();
//        }
//        else {
//            min_x = other.start().x() + other.delta().x();
//            max_x = other.start().x();
//        }
//        if ((x_coord < min_x) || (x_coord > max_x)) {
//            return false;
//        }
//    }

//    // set the result and return success
//    intersection.x() = x_coord;
//    intersection.y() = (line_slope * x_coord) + y_intersect;
//    return true;
//}

// Line2f =========================================================================================================== //

std::ostream& operator<<(std::ostream& out, const Segment2f& segment)
{
    return out << "Line2f((" << segment.start.x() << ", " << segment.start.y() << ") -> (" << segment.end.x() << ", "
               << segment.end.y() << "))";
}

static_assert(sizeof(Segment2f) == sizeof(float) * 4,
              "This compiler seems to inject padding bits into the notf::Line2f memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Segment2f>::value, "This compiler does not recognize notf::Line2f as a POD.");

// Line3f =========================================================================================================== //

std::ostream& operator<<(std::ostream& out, const Segment3f& segment)
{
    return out << "Line3f((" << segment.start.x() << ", " << segment.start.y() << ", " << segment.start.z() << ") -> ("
               << segment.end.x() << ", " << segment.end.y() << ", " << segment.end.z() << "))";
}

static_assert(sizeof(Segment3f) == sizeof(float) * 6,
              "This compiler seems to inject padding bits into the notf::Line3f memory layout. "
              "You may be able to use compiler-specific #pragmas to enforce a contiguous memory layout.");

static_assert(std::is_pod<Segment3f>::value, "This compiler does not recognize notf::Line3f as a POD.");

NOTF_CLOSE_NAMESPACE
