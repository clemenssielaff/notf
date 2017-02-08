#pragma once

#include <iosfwd>
#include <limits>

#include "common/hash_utils.hpp"
#include "common/size2f.hpp"
#include "common/vector2.hpp"

namespace notf {

/** A 2D Axis-Aligned-Bounding-Rectangle.
 * Stores two Vectors, the top-left and bottom-right corner (in a coordinate system where +x is left and +y is down).
 * While this does mean that you need to change four instead of two values for repositioning the Aabr, other
 * calculations (like intersections) are faster; and they are usually more relevant.
 */
struct Aabr {

    /** Top-left corner of the Aabr. */
    Vector2 _min;

    /** Bottom-right corner of the Aabr. */
    Vector2 _max;

    Aabr() = default; // so this data structure remains a POD

    /** Constructs an Aabr of the given width and height, with the top-left corner at the given `x` and `y` coordinates.
     * @param x         X-coordinate of the top-left corner.
     * @param y         Y-coordinate of the top-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    Aabr(float x, float y, float width, float height)
        : _min(x, y)
        , _max(x + width, y + height)
    {
    }

    /** Constructs an Aabr of the given width and height, with the top-left corner at `position`.
     * @param top-left  Position of the Aabr's top-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    Aabr(const Vector2& position, float width, float height)
        : _min(position)
        , _max(position.x + width, position.y + height)
    {
    }

    /** Constructs an Aabr of the given size with the top-left corner at `position`.
     * @param position  Position of the Aabr's top-left corner.
     * @param size      Size of the Aabr.
     */
    Aabr(const Vector2& position, const Size2f& size)
        : _min(position)
        , _max(size.width, size.height)
    {
    }

    /** Aabr with a given width and height, and the top-left corner at zero.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    Aabr(float width, float height)
        : _min(width / -2, height / -2)
        , _max(width / 2, height / 2)
    {
    }

    /** Constructs an Aabr of the given size with the top-left corner at zero.
     * @param size  Size of the Aabr.
     */
    Aabr(const Size2f& size)
        : _min(0, 0)
        , _max(size.width, size.height)
    {
    }

    /** Constructs the Aabr from two of its corners.
     * The corners don't need to be specific, the constructor figures out how to construct an AABR from them.
     * @param a    One corner point of the Aabr.
     * @param b    Opposite corner point of the Aabr.
     */
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

    /* Static Constructors ********************************************************************************************/

    /** The null Aabr. */
    static Aabr null() { return Aabr({0, 0, 0, 0}); }

    /** The largest representable Aabr. */
    static Aabr huge()
    {
        Aabr result;
        result._min = Vector2::fill(std::numeric_limits<float>::lowest());
        result._max = Vector2::fill(std::numeric_limits<float>::max());
        return result;
    }

    /** The "most wrong" Aabr.
     * Is useful as the starting point for defining an Aabr from a set of points.
     */
    static Aabr wrongest()
    {
        Aabr result;
        result._min = Vector2::fill(std::numeric_limits<float>::max());
        result._max = Vector2::fill(std::numeric_limits<float>::lowest());
        return result;
    }

    /*  Inspection  ***************************************************************************************************/

    /** X-coordinate of the center point. */
    float x() const { return (_min.x + _max.x) / 2; }

    /** Y-coordinate of the center point. */
    float y() const { return (_min.y + _max.y) / 2; }

    /** The center of the Aabr. */
    Vector2 center() const { return {x(), y()}; }

    /** X-coordinate of the left edge of this Aabr. */
    float left() const { return _min.x; }

    /** X-coordinate of the right edge of this Aabr. */
    float right() const { return _max.x; }

    /** Y-coordinate of the top edge of this Aabr. */
    float top() const { return _min.y; }

    /** Y-coordinate of the bottom edge of this Aabr. */
    float bottom() const { return _max.y; }

    /** The top left corner of this Aabr. */
    Vector2 top_left() const { return _min; }

    /** The top right corner of this Aabr. */
    Vector2 top_right() const { return {_max.x, _min.y}; }

    /** The bottom left corner of this Aabr. */
    Vector2 bottom_left() const { return {_min.x, _max.y}; }

    /** The bottom right corner of this Aabr. */
    Vector2 bottom_right() const { return _max; }

    /** The width of this Aabr */
    float width() const { return _max.x - _min.x; }

    /** The height of this Aabr */
    float height() const { return _max.y - _min.y; }

    /** The area of this Aabr */
    float area() const { return height() * width(); }

    /** A valid Aabr has a width and height >= 0. */
    bool is_valid() const { return _min.x <= _max.x && _min.y <= _max.y; }

    /** Test, if this Aabr is null.
     * The null Aabr has no area and is located at zero.
     */
    bool is_null() const { return _min.is_zero() && _max.is_zero(); }

    /** Checks if this Aabr contains a given point. */
    bool contains(const Vector2& point) const
    {
        return ((point.x > _min.x) && (point.x < _max.x) && (point.y > _min.y) && (point.y < _max.y));
    }

    /** Checks if two Aabrs intersect.
     * Two Aabrs intersect if their intersection has a non-null area.
     * To get the actual intersection Aabr, use Aabr::intersection().
     */
    bool intersects(const Aabr& other) const
    {
        return !((_max.x < other._min.x) || (_min.x > other._max.x) || (_min.y > other._max.y) || (_max.y < other._min.y));
    }

    /** Returns the closest point inside the Aabr to a given target point.
     * For targets outside the Aabr, the returned point will lay on the Aabr's edge.
     * Targets inside the Aabr are returned unchanged.
     * @param target    Target point.
     * @return          Closest point inside the Aabr to the target point.
     */
    Vector2 closest_point_to(const Vector2& target) const
    {
        const Vector2 pos       = center();
        const float half_width  = width() / 2;
        const float half_height = height() / 2;
        return {pos.x + clamp(target.x - pos.x, -half_width, half_width),
                pos.y + clamp(target.y - pos.y, -half_height, half_height)};
    }

    /** Returns the extend of this Aabr. */
    Size2f extend() const { return {width(), height()}; }

    /** Operators *****************************************************************************************************/

    bool operator==(const Aabr& other) const { return (other._min == _min && other._max == _max); }

    bool operator!=(const Aabr& other) const { return (other._min != _min || other._max != _max); }

    /** Modifications *************************************************************************************************/

    /** Moves this Aabr vertically to the given x-coordinate. */
    Aabr& set_x(float x)
    {
        const float half_width = width() / 2;
        _min.x                 = x - half_width;
        _max.x                 = x + half_width;
        return *this;
    }

    /** Moves this Aabr vertically to the given y-coordinate. */
    Aabr& set_y(float y)
    {
        const float half_height = height() / 2;
        _min.y                  = y - half_height;
        _max.y                  = y + half_height;
        return *this;
    }

    /** Moves this Aabr to a new center position. */
    Aabr& set_center(const Vector2& pos)
    {
        set_x(pos.x);
        return set_y(pos.y);
    }

    /** Sets the x-coordinate of this Aabr's left edge.
     * If the new position is further right than the Aabr's right edge, the right edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    Aabr& set_left(float x)
    {
        _min.x = x;
        _max.x = _max.x > _min.x ? _max.x : _min.x;
        return *this;
    }

    /** Sets the x-coordinate of this Aabr's right edge.
     * If the new position is further left than the Aabr's left edge, the left edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    Aabr& set_right(float x)
    {
        _max.x = x;
        _min.x = _min.x < _max.x ? _min.x : _max.x;
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's top edge.
     * If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    Aabr& set_top(float y)
    {
        _min.y = y;
        _max.y = _max.y > _min.y ? _max.y : _min.y;
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's bottom edge.
     * If the new position is further up than the Aabr's top edge, the top edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    Aabr& set_bottom(float y)
    {
        _max.y = y;
        _min.y = _min.y < _max.y ? _min.y : _max.y;
        return *this;
    }

    /** Sets a new top-left corner of this Aabr.
    * See set_left and set_top for details.
    */
    Aabr& set_top_left(const Vector2& point)
    {
        set_left(point.x);
        return set_top(point.y);
    }

    /** Sets a new top-right corner of this Aabr.
     * See set_right and set_top for details.
     */
    Aabr& set_top_right(const Vector2& point)
    {
        set_right(point.x);
        return set_top(point.y);
    }

    /** Sets a new bottom-left corner of this Aabr.
     * See set_left and set_bottom for details.
     */
    Aabr& set_bottom_left(const Vector2& point)
    {
        set_left(point.x);
        return set_bottom(point.y);
    }

    /** Sets a new bottom-right corner of this Aabr.
     * See set_right and set_bottom for details.
     */
    Aabr& set_bottom_right(const Vector2& point)
    {
        set_right(point.x);
        return set_bottom(point.y);
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a width less than zero is specified, the resulting width is zero.
     */
    Aabr& set_width(float width)
    {
        const float center     = x();
        const float half_width = width / 2;
        _min.x                 = center - half_width;
        _max.x                 = center + half_width;
        return *this;
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a height less than zero is specified, the resulting height is zero.
     */
    Aabr& set_height(float height)
    {
        const float center      = y();
        const float half_height = height / 2;
        _min.y                  = center - half_height;
        _max.y                  = center + half_height;
        return *this;
    }

    /** Sets this Aabr to null. */
    Aabr& set_null()
    {
        _min.set_null();
        _max.set_null();
        return *this;
    }

    /** Moves each edge of the Aabr a given amount towards the outside.
     * Meaning, the width/height of the Aabr will grow by 2*amount.
     * @param amount    Number of units to move each edge.
     */
    Aabr& grow(float amount)
    {
        _min.x -= amount;
        _min.y -= amount;
        _max.x += amount;
        _max.y += amount;
        return *this;
    }

    /** Moves each edge of the Aabr a given amount towards the inside.
     *  Meaning, the width/height of the Aabr will shrink by 2*amount.
     * You cannot shrink the Aabr to negative width or height values.
     * @param amount    Number of units to move each edge.
     */
    Aabr& shrink(float amount) { return grow(-amount); }

    /** Intersection of this Aabr with `other`.
     * Intersecting with another Aabr that does not intersect results in the null Aabr.
     * @return  The intersection Aabr.
     */
    Aabr intersection(const Aabr& other) const
    {
        if (!intersects(other)) {
            return Aabr::null();
        }
        return Aabr(
            Vector2{_min.x > other._min.x ? _min.x : other._min.x, _min.y > other._min.y ? _min.y : other._min.y},
            Vector2{_max.x < other._max.x ? _max.x : other._max.x, _max.y < other._max.y ? _max.y : other._max.y});
    }
    Aabr operator&(const Aabr& other) const { return intersection(other); }

    /** Intersection of this Aabr with `other` in-place.
     * Intersecting with another Aabr that does not intersect results in the null Aabr.
     */
    Aabr& intersected(const Aabr& other)
    {
        if (!intersects(other)) {
            return set_null();
        }
        _min.x = _min.x > other._min.x ? _min.x : other._min.x;
        _max.x = _max.x < other._max.x ? _max.x : other._max.x;
        _min.y = _min.y > other._min.y ? _min.y : other._min.y;
        _max.y = _max.y < other._max.y ? _max.y : other._max.y;
        return *this;
    }
    Aabr operator&=(const Aabr& other) { return intersected(other); }

    /** Creates the union of this Aabr with `other`. */
    Aabr union_(const Aabr& other) const
    {
        return Aabr(
            Vector2{_min.x < other._min.x ? _min.x : other._min.x, _min.y < other._min.y ? _min.y : other._min.y},
            Vector2{_max.x > other._max.x ? _max.x : other._max.x, _max.y > other._max.y ? _max.y : other._max.y});
    }
    Aabr operator|(const Aabr& other) const { return union_(other); }

    /** Creates the union of this Aabr with `other` in-place. */
    Aabr& united(const Aabr& other)
    {
        _min.x = _min.x < other._min.x ? _min.x : other._min.x;
        _min.y = _min.y < other._min.y ? _min.y : other._min.y;
        _max.x = _max.x > other._max.x ? _max.x : other._max.x;
        _max.y = _max.y > other._max.y ? _max.y : other._max.y;
        return *this;
    }
    Aabr& operator|=(const Aabr& other) { return united(other); }

    /** Allows direct memory (read / write) access to the Aabr's internal storage. */
    float* as_ptr() { return &_min.x; }
};

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Aabr into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
std::ostream& operator<<(std::ostream& out, const Aabr& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::Aabr. */
template <>
struct hash<notf::Aabr> {
    size_t operator()(const notf::Aabr& aabr) const { return notf::hash(aabr._min, aabr._max); }
};
}
