#pragma once

#include <iosfwd>

#include "common/vector2.hpp"

namespace notf {

/// @brief A 2D Axis-Aligned-Bounding-Rectangle.
///
/// I finally decided to represent the aabr internally with the vectors: the bottom-left and top-right corner of the
/// Aabr.
/// While it does mean hat you have to change two instead of only one value for repositioning the Aabr, other
/// calculations (like intersection) are faster to perform and since they are usually more important than the position,
/// it seems to be the better choice.
/// Also, everyone else seems to do it that way ...
struct Aabr {

    /// @brief Bottom-left corner of the Aabr.
    Vector2 _min;

    /// @brief Top-right corner of the Aabr.
    Vector2 _max;

    /// Do not implement default methods, so this data structure remains a POD.
    Aabr() = default; // Default Constructor.
    ~Aabr() = default; // Destructor.
    Aabr(const Aabr& other) = default; // Copy Constructor.
    Aabr& operator=(const Aabr& other) = default; // Assignment Operator.

    /// @brief Value Constructor.
    ///
    /// @param position Position of the center of the Aabr
    /// @param width    Width of the Aabr.
    /// @param height   Height of the Aabr.
    Aabr(const Vector2& position, Real width, Real height)
        : _min(position.x - (width / 2), position.y - (height / 2))
        , _max(position.x + (width / 2), position.y + (height / 2))
    {
    }

    /// @brief Value Constructor.
    ///
    /// @param width    Width of the Aabr.
    /// @param height   Height of the Aabr.
    Aabr(Real width, Real height)
        : _min(width / -2, height / -2)
        , _max(width / 2, height / 2)
    {
    }

    /// @brief Value Constructor.
    ///
    /// The corners don't need to be specific, the constructor figures out how to construct an AABR from them.
    ///
    /// @param a    One corner point of the Aabr.
    /// @param b    Opposite corner point of the Aabr.
    Aabr(const Vector2& a, const Vector2& b)
    {
        if (a.x < b.x) {
            if (a.y < b.y) {
                _min = a;
                _max = b;
            }
            else {
                _min = {a.x, b.y};
                _max = {b.x, a.y};
            }
        }
        else {
            if (a.y > b.y) {
                _min = b;
                _max = a;
            }
            else {
                _min = {b.x, a.y};
                _max = {a.x, b.y};
            }
        }
    }

    //  STATIC CONSTRUCTORS  //////////////////////////////////////////////////////////////////////////////////////////

    /// @brief The null Aabr.
    static Aabr null() { return Aabr({0, 0}, {0, 0}); }

    //  INSPECTION  ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief The x-coordinate of this Aabr's position.
    ///
    /// The Aabr's position is at its center.
    Real x() const { return (_min.x + _max.x) / 2; }

    /// @brief The y-coordinate of this Aabr's position.
    ///
    /// The Aabr's position is at its center.
    Real y() const { return (_min.y + _max.y) / 2; }

    /// @brief The position of this Aabr.
    ///
    /// Is always at the center.
    Vector2 position() const { return {x(), y()}; }

    /// @brief X-coordinate of the left edge of this Aabr.
    Real left() const { return _min.x; }

    /// @brief X-coordinate of the right edge of this Aabr.
    Real right() const { return _max.x; }

    /// @brief Y-coordinate of the top edge of this Aabr.
    Real top() const { return _max.y; }

    /// @brief Y-coordinate of the bottom edge of this Aabr.
    Real bottom() const { return _min.y; }

    /// @brief The top left corner of this Aabr.
    Vector2 top_left() const { return {_min.x, _max.y}; }

    /// @brief The top right corner of this Aabr.
    Vector2 top_right() const { return _max; }

    /// @brief The bottom left corner of this Aabr.
    Vector2 bottom_left() const { return _min; }

    /// @brief The bottom right corner of this Aabr.
    Vector2 bottom_right() const { return {_max.x, _min.y}; }

    /// @brief The width of this Aabr
    Real width() const { return _max.x - _min.x; }

    /// @brief The height of this Aabr
    Real height() const { return _max.y - _min.y; }

    /// @brief The area of this Aabr
    Real area() const { return height() * width(); }

    /// @brief Test, if this Aabr is null.
    ///
    /// The null Aabr has no area and is located at zero.
    ///
    /// @return True, iff this Aabr is null.
    bool is_null() const { return _min.is_zero() && _max.is_zero(); }

    /// @brief Checks if this Aabr contains a given point.
    ///
    /// A point on the edge of an Aabr is not contained.
    ///
    /// @param point   Point to test.
    ///
    /// @return True, if the Aabr contains the given point.
    bool contains(const Vector2& point) const
    {
        return ((point.x > left()) && (point.x < right()) && (point.y > bottom()) && (point.y < top()));
    }

    /// @brief Checks if two Aabrs intersect.
    ///
    /// Testing against a Aabr that shares an edge with this one but would not produce an intersection Aabr when
    /// united, results in false being returned.
    /// To get the actual intersection, use Aabr::intersection().
    ///
    /// @param other   Other Aabr to test against.
    ///
    /// @return True if the two Aabrs overlap.
    bool intersects(const Aabr& other) const
    {
        return !((right() < other.left()) || (left() > other.right())
                 || (top() < other.bottom()) || (bottom() > other.top()));
    }

    /// @brief Returns the closest point inside the Aabr to a given target point.
    ///
    /// Both this Aabr and the target point have to be in the same coordinate space.
    /// The result will also be in that space.
    /// For targets outside the Aabr, the returned point will lay on the Aabr's edge.
    /// Targets inside the Aabr are returned unchanged.
    ///
    /// @param target   Target point.
    /// @return Closest point inside the Aabr to the target point.
    Vector2 closest_point_to(const Vector2& target) const
    {
        const Vector2 pos = position();
        const Real half_width = width() / 2;
        const Real half_height = height() / 2;
        return {pos.x + clamp(target.x - pos.x, -half_width, half_width),
                pos.y + clamp(target.y - pos.y, -half_height, half_height)};
    }

    //  OPERATORS  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Sets the new x-coordinate for this Aabr.
    ///
    /// @param x    New x position.
    ///
    /// @return This Aabr after moving.
    Aabr& set_x(Real x)
    {
        const Real half_width = width() / 2;
        _min.x = x - half_width;
        _max.x = x + half_width;
        return *this;
    }

    /// @brief Sets the new y-coordinate for this Aabr.
    ///
    /// @param y    New y position.
    ///
    /// @return This Aabr after moving.
    Aabr& set_y(Real y)
    {
        const Real half_height = height() / 2;
        _min.y = y - half_height;
        _max.y = y + half_height;
        return *this;
    }

    /// @brief Sets the new position of this Aabr.
    ///
    /// @param pos   New position.
    ///
    /// @return This Aabr after moving.
    Aabr& set_position(const Vector2& pos)
    {
        set_x(pos.x);
        return set_y(pos.y);
    }

    /// @brief Sets the x-coordinate of this Aabr's left edge.
    ///
    /// If the new position is further right than the Aabr's right edge, the right edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    ///
    /// @param x    New x-coordinate of the left edge of the Aabr.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_left(Real x)
    {
        _min.x = x;
        _max.x = _min.x < _max.x ? _max.x : _min.x;
        return *this;
    }

    /// @brief Sets the x-coordinate of this Aabr's right edge.
    ///
    /// If the new position is further left than the Aabr's left edge, the left edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    ///
    /// @param x    New x-coordinate of the right edge of the Aabr.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_right(Real x)
    {
        _max.x = x;
        _min.x = _min.x < _max.x ? _min.x : _max.x;
        return *this;
    }

    /// @brief Sets the y-coordinate of this Aabr's top edge.
    ///
    /// If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    ///
    /// @param y    New y-coordinate of the top edge of the Aabr.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_top(Real y)
    {
        _max.y = y;
        _min.y = _min.y < _max.y ? _min.y : _max.y;
        return *this;
    }

    /// @brief Sets the y-coordinate of this Aabr's bottom edge.
    ///
    /// If the new position is further up than the Aabr's top edge, the top edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    ///
    /// @param y    New y-coordinate of the bottom edge of the Aabr.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_bottom(Real y)
    {
        _min.y = y;
        _max.y = _min.y < _max.y ? _max.y : _min.y;
        return *this;
    }

    /// @brief Sets a new top-left corner of this Aabr.
    ///
    /// See set_left and set_top for details.
    ///
    /// @param point New top-left corner.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_top_left(const Vector2& point)
    {
        set_left(point.x);
        return set_top(point.y);
    }

    /// @brief Sets a new top-right corner of this Aabr.
    ///
    /// See set_right and set_top for details.
    ///
    /// @param point New top-right corner.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_top_right(const Vector2& point)
    {
        set_right(point.x);
        return set_top(point.y);
    }

    /// @brief Sets a new bottom-left corner of this Aabr.
    ///
    /// See set_left and set_bottom for details.
    ///
    /// @param point New bottom-left corner.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_bottom_left(const Vector2& point)
    {
        set_left(point.x);
        return set_bottom(point.y);
    }

    /// @brief Sets a new bottom-right corner of this Aabr.
    ///
    /// See set_right and set_bottom for details.
    ///
    /// @param point New bottom-right corner.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_bottom_right(const Vector2& point)
    {
        set_right(point.x);
        return set_bottom(point.y);
    }

    /// @brief Changes the width of this Aabr in place.
    ///
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a width less than zero is specified, the resulting width is zero.
    ///
    /// @param width    New Aabr width.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_width(Real width)
    {
        const Real center = x();
        const Real half_width = width / 2;
        _min.x = center - half_width;
        _max.x = center + half_width;
        return *this;
    }

    /// @brief Changes the height of this Aabr in place.
    ///
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a height less than zero is specified, the resulting height is zero.
    ///
    /// @param height   New Aabr height.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& set_height(Real height)
    {
        const Real center = y();
        const Real half_height = height / 2;
        _min.y = center - half_height;
        _max.y = center + half_height;
        return *this;
    }

    /// @brief Sets this Aabr to be null.
    ///
    /// @return This AAbr after being set to null.
    Aabr& set_null()
    {
        _min.set_zero();
        _max.set_zero();
        return *this;
    }

    //  MODIFIERS  ////////////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Moves each edge of the Aabr a given amount towards the outside.
    ///
    /// Meaning, the width/height of the Aabr will grow by 2*amount.
    ///
    /// @param amount  Number of units to grow.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& grow(Real amount)
    {
        _min.x -= amount;
        _min.y -= amount;
        _max.x += amount;
        _max.y += amount;
        return *this;
    }

    /// @brief Moves each side of the Aabr a given amount towards the inside.
    ///
    /// Meaning, the width/height of the Aabr will shrink by 2*amount.
    /// You cannot shrink the Aabr to negative width or height values.
    ///
    /// @param amount  Number of units to shrink.
    ///
    /// @return This Aabr after the perfoming the operation.
    Aabr& shrink(Real amount) { return grow(-amount); }

    /// @brief Intersection of this Aabr with other.
    ///
    /// Intersecting with another Aabr that does not intersect results in the null Aabr.
    ///
    /// @param other Other Aabr.
    ///
    /// @return The intersection of the two Aabrs.
    Aabr intersection(const Aabr& other)
    {
        if (!intersects(other)) {
            return Aabr::null();
        }
        return Aabr(
            {left() < other.left() ? other.left() : left(), bottom() < other.bottom() ? other.bottom() : bottom()},
            {right() > other.right() ? other.right() : right(), top() > other.top() ? other.top() : top()});
    }
    Aabr operator&(const Aabr& other) { return intersection(other); }

    /// @brief Intersects this Aabr with other in-place.
    ///
    /// Intersecting with another Aabr that does not intersect results in the null Aabr.
    ///
    /// @param other Other Aabr.
    ///
    /// @return This Aabr after the intersection.
    Aabr& intersected(const Aabr& other)
    {
        if (!intersects(other)) {
            return set_null();
        }
        _min.x = left() < other.left() ? other.left() : left();
        _min.y = bottom() < other.bottom() ? other.bottom() : bottom();
        _max.x = right() > other.right() ? other.right() : right();
        _max.y = top() > other.top() ? other.top() : top();
        return *this;
    }
    Aabr operator&=(const Aabr& other) { return intersected(other); }

    /// @brief The union of this Aabr with other.
    ///
    /// @param other    Other Aabr.
    ///
    /// @return The union of the two Aabrs.
    Aabr union_(const Aabr& other)
    {
        return Aabr(
            {left() < other.left() ? left() : other.left(), bottom() < other.bottom() ? bottom() : other.bottom()},
            {right() > other.right() ? right() : other.right(), top() > other.top() ? top() : other.top()});
    }
    Aabr operator|(const Aabr& other) { return union_(other); }

    /// @brief Unites this Aabr union with other in-place.
    ///
    /// @param other    Other Aabr.
    ///
    /// @return This Aabr after uniting with other.
    Aabr& united(const Aabr& other)
    {
        _min.x = left() < other.left() ? left() : other.left();
        _min.y = bottom() < other.bottom() ? bottom() : other.bottom();
        _max.x = right() > other.right() ? right() : other.right();
        _max.y = top() > other.top() ? top() : other.top();
        return *this;
    }
    Aabr& operator|=(const Aabr& other) { return united(other); }
};

//  FREE FUNCTIONS  ///////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Prints the contents of this Aabr into a std::ostream.
///
/// @param os   Output stream, implicitly passed with the << operator.
/// @param aabr Aabr to print.
///
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Aabr& aabr);

} // namespace notf
