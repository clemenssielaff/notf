#pragma once

#include <iosfwd>
#include <limits>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"
#include "utils/sfinae.hpp"

namespace notf {

//*********************************************************************************************************************/

/** A 2D Axis-Aligned-Bounding-Rectangle.
 * Stores two Vectors, the top-left and bottom-right corner (in a coordinate system where +x is left and +y is down).
 * While this does mean that you need to change four instead of two values for repositioning the Aabr, other
 * calculations (like intersections) are faster; and they are usually more relevant.
 */
// clang-format off
template <typename Vector2,
          typename = std::enable_if_t<std::is_base_of<Vector2f, typename std::decay<Vector2>::type>::value ||
                                      std::is_base_of<Vector2d, typename std::decay<Vector2>::type>::value ||
                                      std::is_base_of<Vector2i, typename std::decay<Vector2>::type>::value>>
// clang-format on
struct _Aabr {

    /* Types **********************************************************************************************************/

    using Value_t = typename Vector2::Value_t;

    using Vector_t = Vector2;

    /* Fields *********************************************************************************************************/

    /** Top-left corner of the Aabr. */
    Vector_t _min;

    /** Bottom-right corner of the Aabr. */
    Vector_t _max;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Aabr() = default;

    /** Constructs an Aabr of the given width and height, with the top-left corner at the given `x` and `y` coordinates.
     * @param x         X-coordinate of the top-left corner.
     * @param y         Y-coordinate of the top-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const Value_t x, const Value_t y, const Value_t width, const Value_t height)
        : _min(x, y)
        , _max(x + width, y + height)
    {
    }

    /** Constructs an Aabr of the given width and height, with the top-left corner at `position`.
     * @param top-left  Position of the Aabr's top-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const Vector_t& position, const Value_t width, const Value_t height)
        : _min(position)
        , _max(position.x + width, position.y + height)
    {
    }

    /** Constructs an Aabr of the given size with the top-left corner at `position`.
     * @param position  Position of the Aabr's top-left corner.
     * @param size      Size of the Aabr.
     */
    _Aabr(const Vector_t& position, const Size2f& size)
        : _min(position)
        , _max(position.x + size.width, position.y + size.height)
    {
    }

    /** Aabr with a given width and height, and the top-left corner at zero.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const Value_t width, const Value_t height)
        : _min(0, 0)
        , _max(width, height)
    {
    }

    /** Constructs an Aabr of the given size with the top-left corner at zero.
     * @param size  Size of the Aabr.
     */
    _Aabr(const _Size2<Value_t>& size)
        : _min(0, 0)
        , _max(size.width, size.height)
    {
    }

    /** Constructs the Aabr from two of its corners.
     * The corners don't need to be specific, the constructor figures out how to construct an AABR from them.
     * @param a    One corner point of the Aabr.
     * @param b    Opposite corner point of the Aabr.
     */
    _Aabr(const Vector_t& a, const Vector_t& b)
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
    static _Aabr null() { return _Aabr({0, 0, 0, 0}); }

    /** The largest representable Aabr. */
    static _Aabr huge()
    {
        _Aabr result;
        result._min = Vector_t::fill(std::numeric_limits<Value_t>::lowest());
        result._max = Vector_t::fill(std::numeric_limits<Value_t>::max());
        return result;
    }

    /** The "most wrong" Aabr.
     * Is useful as the starting point for defining an Aabr from a set of points.
     */
    static _Aabr wrongest()
    {
        _Aabr result;
        result._min = Vector_t::fill(std::numeric_limits<Value_t>::max());
        result._max = Vector_t::fill(std::numeric_limits<Value_t>::lowest());
        return result;
    }

    /*  Inspection  ***************************************************************************************************/

    /** X-coordinate of the center point. */
    Value_t x() const { return (_min.x + _max.x) / 2; }

    /** Y-coordinate of the center point. */
    Value_t y() const { return (_min.y + _max.y) / 2; }

    /** The center of the Aabr. */
    Vector_t center() const { return {x(), y()}; }

    /** X-coordinate of the left edge of this Aabr. */
    Value_t left() const { return _min.x; }

    /** X-coordinate of the right edge of this Aabr. */
    Value_t right() const { return _max.x; }

    /** Y-coordinate of the top edge of this Aabr. */
    Value_t top() const { return _min.y; }

    /** Y-coordinate of the bottom edge of this Aabr. */
    Value_t bottom() const { return _max.y; }

    /** The top left corner of this Aabr. */
    Vector_t top_left() const { return _min; }

    /** The top right corner of this Aabr. */
    Vector_t top_right() const { return {_max.x, _min.y}; }

    /** The bottom left corner of this Aabr. */
    Vector_t bottom_left() const { return {_min.x, _max.y}; }

    /** The bottom right corner of this Aabr. */
    Vector_t bottom_right() const { return _max; }

    /** The width of this Aabr */
    Value_t width() const { return _max.x - _min.x; }

    /** The height of this Aabr */
    Value_t height() const { return _max.y - _min.y; }

    /** The area of this Aabr */
    Value_t area() const { return height() * width(); }

    /** A valid Aabr has a width and height >= 0. */
    bool is_valid() const { return _min.x <= _max.x && _min.y <= _max.y; }

    /** Test, if this Aabr is null.
     * The null Aabr has no area and is located at zero.
     */
    bool is_null() const { return _min.is_zero() && _max.is_zero(); }

    /** Checks if this Aabr contains a given point. */
    bool contains(const Vector_t& point) const
    {
        return ((point.x > _min.x) && (point.x < _max.x) && (point.y > _min.y) && (point.y < _max.y));
    }

    /** Checks if two Aabrs intersect.
     * Two Aabrs intersect if their intersection has a non-null area.
     * To get the actual intersection Aabr, use Aabr::intersection().
     */
    bool intersects(const _Aabr& other) const
    {
        return !((_max.x < other._min.x) || (_min.x > other._max.x) || (_min.y > other._max.y) || (_max.y < other._min.y));
    }

    /** Returns the closest point inside the Aabr to a given target point.
     * For targets outside the Aabr, the returned point will lay on the Aabr's edge.
     * Targets inside the Aabr are returned unchanged.
     * @param target    Target point.
     * @return          Closest point inside the Aabr to the target point.
     */
    Vector_t closest_point_to(const Vector_t& target) const
    {
        const Vector_t pos        = center();
        const Value_t half_width  = width() / 2;
        const Value_t half_height = height() / 2;
        return {pos.x + clamp(target.x - pos.x, -half_width, half_width),
                pos.y + clamp(target.y - pos.y, -half_height, half_height)};
    }

    /** Returns the extend of this Aabr. */
    Size2f extend() const { return {width(), height()}; }

    /** Returns the length of the longer side of this Aabr. */
    Value_t longer_size() const
    {
        const Value_t width  = this->width();
        const Value_t height = this->height();
        return width > height ? width : height;
    }

    /** Returns the length of the shorter side of this Aabr. */
    Value_t shorter_size() const
    {
        const Value_t width  = this->width();
        const Value_t height = this->height();
        return width < height ? width : height;
    }

    /** Read-only pointer to the Aabr's internal storage. */
    const Value_t* as_ptr() const { return &_min.x; }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Aabrs are equal. */
    bool operator==(const _Aabr& other) const { return (other._min == _min && other._max == _max); }

    /** Tests whether two Aabrs are not equal. */
    bool operator!=(const _Aabr& other) const { return (other._min != _min || other._max != _max); }

    /** Multiplication with a scalar scales the Aabr in relation to local zero. */
    _Aabr operator*(const Value_t factor) { return {_min * factor, _max * factor}; }

    /** Adding a Vector_t to an Aabr moves the Aabr relative to its previous position. */
    _Aabr operator+(const Vector_t& offset) { return {_min + offset, _max + offset}; }

    /** Modifiers *****************************************************************************************************/

    /** Moves this Aabr vertically to the given x-coordinate. */
    _Aabr& set_x(const Value_t x)
    {
        const Value_t half_width = width() / 2;
        _min.x                   = x - half_width;
        _max.x                   = x + half_width;
        return *this;
    }

    /** Moves this Aabr vertically to the given y-coordinate. */
    _Aabr& set_y(const Value_t y)
    {
        const Value_t half_height = height() / 2;
        _min.y                    = y - half_height;
        _max.y                    = y + half_height;
        return *this;
    }

    /** Moves this Aabr to a new center position. */
    _Aabr& set_center(const Vector_t& pos)
    {
        set_x(pos.x);
        return set_y(pos.y);
    }

    /** Moves this Aabr by a relative amount. */
    _Aabr& move_by(const Vector_t& pos)
    {
        _min += pos;
        _max += pos;
        return *this;
    }

    /** Sets the x-coordinate of this Aabr's left edge.
     * If the new position is further right than the Aabr's right edge, the right edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    _Aabr& set_left(const Value_t x)
    {
        _min.x = x;
        _max.x = _max.x > _min.x ? _max.x : _min.x;
        return *this;
    }

    /** Sets the x-coordinate of this Aabr's right edge.
     * If the new position is further left than the Aabr's left edge, the left edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    _Aabr& set_right(const Value_t x)
    {
        _max.x = x;
        _min.x = _min.x < _max.x ? _min.x : _max.x;
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's top edge.
     * If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    _Aabr& set_top(const Value_t y)
    {
        _min.y = y;
        _max.y = _max.y > _min.y ? _max.y : _min.y;
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's bottom edge.
     * If the new position is further up than the Aabr's top edge, the top edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    _Aabr& set_bottom(const Value_t y)
    {
        _max.y = y;
        _min.y = _min.y < _max.y ? _min.y : _max.y;
        return *this;
    }

    /** Sets a new top-left corner of this Aabr.
    * See set_left and set_top for details.
    */
    _Aabr& set_top_left(const Vector_t& point)
    {
        set_left(point.x);
        return set_top(point.y);
    }

    /** Sets a new top-right corner of this Aabr.
     * See set_right and set_top for details.
     */
    _Aabr& set_top_right(const Vector_t& point)
    {
        set_right(point.x);
        return set_top(point.y);
    }

    /** Sets a new bottom-left corner of this Aabr.
     * See set_left and set_bottom for details.
     */
    _Aabr& set_bottom_left(const Vector_t& point)
    {
        set_left(point.x);
        return set_bottom(point.y);
    }

    /** Sets a new bottom-right corner of this Aabr.
     * See set_right and set_bottom for details.
     */
    _Aabr& set_bottom_right(const Vector_t& point)
    {
        set_right(point.x);
        return set_bottom(point.y);
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a width less than zero is specified, the resulting width is zero.
     */
    _Aabr& set_width(const Value_t width)
    {
        const Value_t center     = x();
        const Value_t half_width = width / 2;
        _min.x                   = center - half_width;
        _max.x                   = center + half_width;
        return *this;
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a height less than zero is specified, the resulting height is zero.
     */
    _Aabr& set_height(const Value_t height)
    {
        const Value_t center      = y();
        const Value_t half_height = height / 2;
        _min.y                    = center - half_height;
        _max.y                    = center + half_height;
        return *this;
    }

    /** Sets this Aabr to null. */
    _Aabr& set_null()
    {
        _min.set_zero();
        _max.set_zero();
        return *this;
    }

    /** Moves each edge of the Aabr a given amount towards the outside.
     * Meaning, the width/height of the Aabr will grow by 2*amount.
     * @param amount    Number of units to move each edge.
     */
    _Aabr& grow(const Value_t amount)
    {
        _min.x -= amount;
        _min.y -= amount;
        _max.x += amount;
        _max.y += amount;
        return *this;
    }

    /** Returns a grown copy of this Aabr. */
    _Aabr grown(const Value_t amount) const
    {
        _Aabr result(*this);
        result.grow(amount);
        return result;
    }

    /** Moves each edge of the Aabr a given amount towards the inside.
     *  Meaning, the width/height of the Aabr will shrink by 2*amount.
     * You cannot shrink the Aabr to negative width or height values.
     * @param amount    Number of units to move each edge.
     */
    _Aabr& shrink(const Value_t amount) { return grow(-amount); }

    /** Returns a shrunken copy of this Aabr. */
    _Aabr shrunken(const Value_t amount) const
    {
        _Aabr result(*this);
        result.grow(-amount);
        return result;
    }

    /** Intersection of this Aabr with `other`.
     * Intersecting with another Aabr that does not intersect results in the null Aabr.
     * @return  The intersection Aabr.
     */
    _Aabr intersection(const _Aabr& other) const
    {
        if (!intersects(other)) {
            return _Aabr::null();
        }
        return _Aabr(
            Vector_t{_min.x > other._min.x ? _min.x : other._min.x, _min.y > other._min.y ? _min.y : other._min.y},
            Vector_t{_max.x < other._max.x ? _max.x : other._max.x, _max.y < other._max.y ? _max.y : other._max.y});
    }
    _Aabr operator&(const _Aabr& other) const { return intersection(other); }

    /** Intersection of this Aabr with `other` in-place.
     * Intersecting with another Aabr that does not intersect results in the null Aabr.
     */
    _Aabr& intersect(const _Aabr& other)
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
    _Aabr operator&=(const _Aabr& other) { return intersect(other); }

    /** Creates the union of this Aabr with `other`. */
    _Aabr union_(const _Aabr& other) const
    {
        return _Aabr(
            Vector_t{_min.x < other._min.x ? _min.x : other._min.x, _min.y < other._min.y ? _min.y : other._min.y},
            Vector_t{_max.x > other._max.x ? _max.x : other._max.x, _max.y > other._max.y ? _max.y : other._max.y});
    }
    _Aabr operator|(const _Aabr& other) const { return union_(other); }

    /** Creates the union of this Aabr with `other` in-place. */
    _Aabr& united(const _Aabr& other)
    {
        _min.x = _min.x < other._min.x ? _min.x : other._min.x;
        _min.y = _min.y < other._min.y ? _min.y : other._min.y;
        _max.x = _max.x > other._max.x ? _max.x : other._max.x;
        _max.y = _max.y > other._max.y ? _max.y : other._max.y;
        return *this;
    }
    _Aabr& operator|=(const _Aabr& other) { return united(other); }

    /** Returns the Aabr of this Aabr rotated counter-clockwise (in radians) around its center. */
    _Aabr get_rotated(const Value_t angle) const { return get_rotated(angle, center()); }

    /** Returns the Aabr of this Aabr rotated counter-clockwise (in radians) around a pivot point. */
    _Aabr get_rotated(const Value_t angle, const Vector_t& pivot) const
    {
        const Vector_t a = _min.get_rotated_around(angle, pivot);
        const Vector_t b = top_right().rotate_around(angle, pivot);
        const Vector_t c = _max.get_rotated_around(angle, pivot);
        const Vector_t d = bottom_left().rotate_around(angle, pivot);
        _Aabr result;
        result._min.x = min(a.x, min(b.x, min(c.x, d.x)));
        result._min.y = min(a.y, min(b.y, min(c.y, d.y)));
        result._max.x = max(a.x, max(b.x, max(c.x, d.x)));
        result._max.y = max(a.y, max(b.y, max(c.y, d.y)));
        return result;
    }

    /** Read-write pointer to the Aabr's internal storage. */
    Value_t* as_ptr() { return &_min.x; }
};

//*********************************************************************************************************************/

using Aabrf = _Aabr<Vector2f>;

/* Free Functions *****************************************************************************************************/

/** Prints the contents of this Aabr into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
template <typename Vector2>
std::ostream& operator<<(std::ostream& out, const notf::_Aabr<Vector2>& aabr);

} // namespace notf

/* std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_Aabr. */
template <typename Vector2>
struct hash<notf::_Aabr<Vector2>> {
    size_t operator()(const notf::_Aabr<Vector2>& aabr) const { return notf::hash(aabr._min, aabr._max); }
};
}
