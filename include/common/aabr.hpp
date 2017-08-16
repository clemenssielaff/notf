#pragma once

#include <iosfwd>
#include <limits>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/meta.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"
#include "common/xform2.hpp"
#include "common/xform3.hpp"

namespace notf {

//*********************************************************************************************************************/

/** A 2D Axis-Aligned-Bounding-Rectangle.
 * Stores two Vectors, the bottom-left and top-right corner.
 * While this does mean that you need to change four instead of two values for repositioning the Aabr, other
 * calculations (like intersections) are faster; and they are usually more relevant.
 */
template <typename VECTOR2>
struct _Aabr {

    /* Types **********************************************************************************************************/

    using value_t = typename VECTOR2::value_t;

    using vector_t = VECTOR2;

    /* Fields *********************************************************************************************************/

    /** Bottom-left corner of the Aabr. */
    vector_t _min;

    /** Top-right corner of the Aabr. */
    vector_t _max;

    /* Constructors ***************************************************************************************************/

    /** Default (non-initializing) constructor so this struct remains a POD */
    _Aabr() = default;

    /** Constructs an Aabr of the given width and height, with the bottom-left corner at the given coordinates.
     * @param x         X-coordinate of the bottom-left corner.
     * @param y         Y-coordinate of the bottom-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const value_t x, const value_t y, const value_t width, const value_t height)
        : _min(x, y)
        , _max(x + width, y + height)
    {
    }

    /** Constructs an Aabr of the given width and height, with the bottom-left corner at `position`.
     * @param position  Position of the Aabr's bottom-left corner.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const vector_t& position, const value_t width, const value_t height)
        : _min(position)
        , _max(position.x() + width, position.y() + height)
    {
    }

    /** Constructs an Aabr of the given size with the bottom-left corner at `position`.
     * @param position  Position of the Aabr's bottom-left corner.
     * @param size      Size of the Aabr.
     */
    _Aabr(const vector_t& position, const Size2f& size)
        : _min(position)
        , _max(position.x() + size.width, position.y() + size.height)
    {
    }

    /** Aabr with a given width and height, and the bottom-left corner at zero.
     * @param width     Width of the Aabr.
     * @param height    Height of the Aabr.
     */
    _Aabr(const value_t width, const value_t height)
        : _min(0, 0)
        , _max(width, height)
    {
    }

    /** Constructs an Aabr of the given size with the bottom-left corner at zero.
     * @param size  Size of the Aabr.
     */
    _Aabr(const _Size2<value_t>& size)
        : _min(0, 0)
        , _max(size.width, size.height)
    {
    }

    /** Constructs the Aabr from two of its corners.
     * The corners don't need to be specific, the constructor figures out how to construct an Aabr from them.
     * @param a    One corner point of the Aabr.
     * @param b    Opposite corner point of the Aabr.
     */
    _Aabr(const vector_t& a, const vector_t& b)
    {
        if (a.x() < b.x()) {
            if (a.y() < b.y()) {
                _min = a;
                _max = b;
            }
            else {
                _min = {a.x(), b.y()};
                _max = {b.x(), a.y()};
            }
        }
        else {
            if (a.y() > b.y()) {
                _min = b;
                _max = a;
            }
            else {
                _min = {b.x(), a.y()};
                _max = {a.x(), b.y()};
            }
        }
    }

    /* Static Constructors ********************************************************************************************/

    /** The zero Aabr. */
    static _Aabr zero() { return _Aabr({0, 0, 0, 0}); }

    /** The largest representable Aabr. */
    static _Aabr huge()
    {
        _Aabr result;
        result._min = vector_t::fill(std::numeric_limits<value_t>::lowest());
        result._max = vector_t::fill(std::numeric_limits<value_t>::max());
        return result;
    }

    /** The "most wrong" Aabr (maximal negative area).
     * Is useful as the starting point for defining an Aabr from a set of points.
     */
    static _Aabr wrongest()
    {
        _Aabr result;
        result._min = vector_t::fill(std::numeric_limits<value_t>::max());
        result._max = vector_t::fill(std::numeric_limits<value_t>::lowest());
        return result;
    }

    /** Returns an Aabr of a given size, with zero in the center. */
    static _Aabr centered(const _Size2<value_t>& size)
    {
        const value_t half_width  = size.width / 2;
        const value_t half_height = size.height / 2;
        _Aabr result;
        result._min = {-half_width, -half_height};
        result._max = {half_width, half_height};
        return result;
    }

    /*  Inspection  ***************************************************************************************************/

    /** Creates a copy of this Aabr. */
    _Aabr copy() const { return *this; }

    /** X-coordinate of the center point. */
    value_t x() const { return (_min.x() + _max.x()) / 2; }

    /** Y-coordinate of the center point. */
    value_t y() const { return (_min.y() + _max.y()) / 2; }

    /** The center of the Aabr. */
    vector_t center() const { return {x(), y()}; }

    /** X-coordinate of the left edge of this Aabr. */
    value_t left() const { return _min.x(); }

    /** X-coordinate of the right edge of this Aabr. */
    value_t right() const { return _max.x(); }

    /** Y-coordinate of the top edge of this Aabr. */
    value_t top() const { return _max.y(); }

    /** Y-coordinate of the bottom edge of this Aabr. */
    value_t bottom() const { return _min.y(); }

    /** The bottom left corner of this Aabr. */
    const vector_t& bottom_left() const { return _min; }

    /** The top right corner of this Aabr. */
    const vector_t& top_right() const { return _max; }

    /** The top left corner of this Aabr. */
    vector_t top_left() const { return {_min.x(), _max.y()}; }

    /** The bottom right corner of this Aabr. */
    vector_t bottom_right() const { return {_max.x(), _min.y()}; }

    /** The width of this Aabr */
    value_t get_width() const { return _max.x() - _min.x(); }

    /** The height of this Aabr */
    value_t get_height() const { return _max.y() - _min.y(); }

    /** The area of this Aabr */
    value_t get_area() const { return get_height() * get_width(); }

    /** A valid Aabr has a width and height >= 0. */
    bool is_valid() const { return _min.x() <= _max.x() && _min.y() <= _max.y(); }

    /** Test, if this Aabr is zero.
     * The zero Aabr has no area and is located at zero.
     */
    bool is_zero() const { return _min.is_zero() && _max.is_zero(); }

    /** Checks if this Aabr contains a given point. */
    bool contains(const vector_t& point) const
    {
        return ((point.x() > _min.x()) && (point.x() < _max.x()) && (point.y() > _min.y()) && (point.y() < _max.y()));
    }

    /** Checks if two Aabrs intersect.
     * Two Aabrs intersect if their intersection has a non-zero area.
     * To get the actual intersection Aabr, use Aabr::intersection().
     */
    bool intersects(const _Aabr& other) const
    {
        return !((_max.x() < other._min.x()) || (_min.x() > other._max.x())
                 || (_min.y() > other._max.y()) || (_max.y() < other._min.y()));
    }

    /** Returns the closest point inside the Aabr to a given target point.
     * For targets outside the Aabr, the returned point will lay on the Aabr's edge.
     * Targets inside the Aabr are returned unchanged.
     * @param target    Target point.
     * @return          Closest point inside the Aabr to the target point.
     */
    vector_t closest_point_to(const vector_t& target) const
    {
        const vector_t pos        = center();
        const value_t half_width  = get_width() / 2;
        const value_t half_height = get_height() / 2;
        return {pos.x() + clamp(target.x() - pos.x(), -half_width, half_width),
                pos.y() + clamp(target.y() - pos.y(), -half_height, half_height)};
    }

    /** Returns the extend of this Aabr. */
    Size2f get_size() const { return {get_width(), get_height()}; }

    /** Returns the length of the longer side of this Aabr. */
    value_t get_longer_side() const
    {
        const value_t width  = this->get_width();
        const value_t height = this->get_height();
        return width > height ? width : height;
    }

    /** Returns the length of the shorter side of this Aabr. */
    value_t get_shorter_side() const
    {
        const value_t width  = this->get_width();
        const value_t height = this->get_height();
        return width < height ? width : height;
    }

    /** Read-only pointer to the Aabr's internal storage. */
    const value_t* as_ptr() const { return &_min.x(); }

    /** Operators *****************************************************************************************************/

    /** Tests whether two Aabrs are equal. */
    bool operator==(const _Aabr& other) const { return (other._min == _min && other._max == _max); }

    /** Tests whether two Aabrs are not equal. */
    bool operator!=(const _Aabr& other) const { return (other._min != _min || other._max != _max); }

    /** Multiplication with a scalar scales the Aabr in relation to local zero. */
    _Aabr operator*(const value_t factor) { return {_min * factor, _max * factor}; }

    /** Adding a vector_t to an Aabr moves the Aabr relative to its previous position. */
    _Aabr operator+(const vector_t& offset) { return {_min + offset, _max + offset}; }

    /** Modifiers *****************************************************************************************************/

    /** Moves the center of this Aabr to the given x-coordinate. */
    _Aabr& set_x(const value_t x)
    {
        const value_t half_width = get_width() / 2;
        _min.x()                 = x - half_width;
        _max.x()                 = x + half_width;
        return *this;
    }

    /** Moves the center of this Aabr to the given y-coordinate. */
    _Aabr& set_y(const value_t y)
    {
        const value_t half_height = get_height() / 2;
        _min.y()                  = y - half_height;
        _max.y()                  = y + half_height;
        return *this;
    }

    /** Moves this Aabr to a new center position. */
    _Aabr& set_center(const vector_t& pos)
    {
        set_x(pos.x());
        return set_y(pos.y());
    }

    /** Moves this Aabr by a relative amount. */
    _Aabr& move_by(const vector_t& pos)
    {
        _min += pos;
        _max += pos;
        return *this;
    }

    /** Sets the x-coordinate of this Aabr's left edge.
     * If the new position is further right than the Aabr's right edge, the right edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    _Aabr& set_left(const value_t x)
    {
        _min.x() = x;
        _max.x() = _max.x() > _min.x() ? _max.x() : _min.x();
        return *this;
    }

    /** Sets the x-coordinate of this Aabr's right edge.
     * If the new position is further left than the Aabr's left edge, the left edge is moved to the same
     * position, resulting in a Aabr with zero width.
     */
    _Aabr& set_right(const value_t x)
    {
        _max.x() = x;
        _min.x() = _min.x() < _max.x() ? _min.x() : _max.x();
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's top edge.
     * If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    _Aabr& set_top(const value_t y)
    {
        _max.y() = y;
        _min.y() = _min.y() < _max.y() ? _min.y() : _max.y();
        return *this;
    }

    /** Sets the y-coordinate of this Aabr's bottom edge.
     * If the new position is further up than the Aabr's top edge, the top edge is moved to the same
     * position, resulting in a Aabr with zero height.
     */
    _Aabr& set_bottom(const value_t y)
    {
        _min.y() = y;
        _max.y() = _max.y() > _min.y() ? _max.y() : _min.y();
        return *this;
    }

    /** Sets a new top-left corner of this Aabr.
    * See set_left and set_top for details.
    */
    _Aabr& set_top_left(const vector_t& point)
    {
        set_left(point.x());
        return set_top(point.y());
    }

    /** Sets a new top-right corner of this Aabr.
     * See set_right and set_top for details.
     */
    _Aabr& set_top_right(const vector_t& point)
    {
        set_right(point.x());
        return set_top(point.y());
    }

    /** Sets a new bottom-left corner of this Aabr.
     * See set_left and set_bottom for details.
     */
    _Aabr& set_bottom_left(const vector_t& point)
    {
        set_left(point.x());
        return set_bottom(point.y());
    }

    /** Sets a new bottom-right corner of this Aabr.
     * See set_right and set_bottom for details.
     */
    _Aabr& set_bottom_right(const vector_t& point)
    {
        set_right(point.x());
        return set_bottom(point.y());
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a width less than zero is specified, the resulting width is zero.
     */
    _Aabr& set_width(const value_t width)
    {
        const value_t center     = x();
        const value_t half_width = width / 2;
        _min.x()                 = center - half_width;
        _max.x()                 = center + half_width;
        return *this;
    }

    /** Changes the height of this Aabr in place.
     * The scaling occurs from the center of the Aabr, meaning its position does not change.
     * If a height less than zero is specified, the resulting height is zero.
     */
    _Aabr& set_height(const value_t height)
    {
        const value_t center      = y();
        const value_t half_height = height / 2;
        _min.y()                  = center - half_height;
        _max.y()                  = center + half_height;
        return *this;
    }

    /** Sets this Aabr to zero. */
    _Aabr& set_zero()
    {
        _min.set_zero();
        _max.set_zero();
        return *this;
    }

    /** Moves each edge of the Aabr a given amount towards the outside.
     * Meaning, the width/height of the Aabr will grow by 2*amount.
     * @param amount    Number of units to move each edge.
     */
    _Aabr& grow(const value_t amount)
    {
        _min.x() -= amount;
        _min.y() -= amount;
        _max.x() += amount;
        _max.y() += amount;
        return *this;
    }

    /** Returns a grown copy of this Aabr. */
    _Aabr grow(const value_t amount) const
    {
        _Aabr result(*this);
        result.grow(amount);
        return result;
    }

    /** Grows this Aabr to include the given point.
     * If the point is already within the rectangle, this does nothing.
     * @param point     Point to include in the Aabr.
     */
    _Aabr& grow_to(const vector_t& point)
    {
        _min.x() = min(_min.x(), point.x());
        _min.y() = min(_min.y(), point.y());
        _max.x() = max(_max.x(), point.x());
        _max.y() = max(_max.y(), point.y());
        return *this;
    }

    /** Moves each edge of the Aabr a given amount towards the inside.
     *  Meaning, the width/height of the Aabr will shrink by 2*amount.
     * You cannot shrink the Aabr to negative width or height values.
     * @param amount    Number of units to move each edge.
     */
    _Aabr& shrink(const value_t amount) { return grow(-amount); }

    /** Returns a shrunken copy of this Aabr. */
    _Aabr shrink(const value_t amount) const
    {
        _Aabr result(*this);
        result.shrink(amount);
        return result;
    }

    /** Intersection of this Aabr with `other`.
     * Intersecting with another Aabr that does not intersect results in the zero Aabr.
     * @return  The intersection Aabr.
     */
    _Aabr intersect(const _Aabr& other) const
    {
        if (!intersects(other)) {
            return _Aabr::zero();
        }
        return _Aabr(
            vector_t{_min.x() > other._min.x() ? _min.x() : other._min.x(), _min.y() > other._min.y() ? _min.y() : other._min.y()},
            vector_t{_max.x() < other._max.x() ? _max.x() : other._max.x(), _max.y() < other._max.y() ? _max.y() : other._max.y()});
    }
    _Aabr operator&(const _Aabr& other) const { return intersect(other); }

    /** Intersection of this Aabr with `other` in-place.
     * Intersecting with another Aabr that does not intersect results in the zero Aabr.
     */
    _Aabr& intersect(const _Aabr& other)
    {
        if (!intersects(other)) {
            return set_zero();
        }
        _min.x() = _min.x() > other._min.x() ? _min.x() : other._min.x();
        _max.x() = _max.x() < other._max.x() ? _max.x() : other._max.x();
        _min.y() = _min.y() > other._min.y() ? _min.y() : other._min.y();
        _max.y() = _max.y() < other._max.y() ? _max.y() : other._max.y();
        return *this;
    }
    _Aabr operator&=(const _Aabr& other) { return intersect(other); }

    /** Creates the union of this Aabr with `other`. */
    _Aabr unite(const _Aabr& other) const
    {
        return _Aabr(
            vector_t{_min.x() < other._min.x() ? _min.x() : other._min.x(), _min.y() < other._min.y() ? _min.y() : other._min.y()},
            vector_t{_max.x() > other._max.x() ? _max.x() : other._max.x(), _max.y() > other._max.y() ? _max.y() : other._max.y()});
    }
    _Aabr operator|(const _Aabr& other) const { return unite(other); }

    /** Creates the union of this Aabr with `other` in-place. */
    _Aabr& unite(const _Aabr& other)
    {
        _min.x() = _min.x() < other._min.x() ? _min.x() : other._min.x();
        _min.y() = _min.y() < other._min.y() ? _min.y() : other._min.y();
        _max.x() = _max.x() > other._max.x() ? _max.x() : other._max.x();
        _max.y() = _max.y() > other._max.y() ? _max.y() : other._max.y();
        return *this;
    }
    _Aabr& operator|=(const _Aabr& other) { return unite(other); }

    /** Read-write pointer to the Aabr's internal storage. */
    value_t* as_ptr() { return &_min.x(); }

    /** Applies a transformation to this Aabr in-place. */
    template <typename XFORM>
    _Aabr& transformed_by(const XFORM& xform)
    {
        typename XFORM::vector_t d0(_min);
        typename XFORM::vector_t d1(_max);
        typename XFORM::vector_t d2(vector_t{_min.x(), _max.y()});
        typename XFORM::vector_t d3(vector_t{_max.x(), _min.y()});
        xform.transform(d0);
        xform.transform(d1);
        xform.transform(d2);
        xform.transform(d3);
        _min.x() = min(d0.x(), d1.x(), d2.x(), d3.x());
        _min.y() = min(d0.y(), d1.y(), d2.y(), d3.y());
        _max.x() = max(d0.x(), d1.x(), d2.x(), d3.x());
        _max.y() = max(d0.y(), d1.y(), d2.y(), d3.y());
        return *this;
    }
};

//*********************************************************************************************************************/

using Aabrf = _Aabr<Vector2f>;
using Aabrd = _Aabr<Vector2d>;

// Transform specializations ******************************************************************************************/

namespace detail {

template <>
inline Aabrf& transform2(const Xform2f& xform, Aabrf& aabr) { return aabr.transformed_by(xform); }
template <>
inline Aabrf transform2(const Xform2f& xform, const Aabrf& aabr) { return aabr.copy().transformed_by(xform); }

template <>
inline Aabrd& transform2(const Xform2d& xform, Aabrd& aabr) { return aabr.transformed_by(xform); }
template <>
inline Aabrd transform2(const Xform2d& xform, const Aabrd& aabr) { return aabr.copy().transformed_by(xform); }

template <>
inline Aabrf& transform3(const Xform3f& xform, Aabrf& aabr) { return aabr.transformed_by(xform); }
template <>
inline Aabrf transform3(const Xform3f& xform, const Aabrf& aabr) { return aabr.copy().transformed_by(xform); }

template <>
inline Aabrd& transform3(const Xform3d& xform, Aabrd& aabr) { return aabr.transformed_by(xform); }
template <>
inline Aabrd transform3(const Xform3d& xform, const Aabrd& aabr) { return aabr.copy().transformed_by(xform); }

} // namespace detail

// Free Functions *****************************************************************************************************/

/** Prints the contents of this Aabr into a std::ostream.
 * @param out   Output stream, implicitly passed with the << operator.
 * @param aabr  Aabr to print.
 * @return      Output stream for further output.
 */
template <typename VECTOR2>
std::ostream& operator<<(std::ostream& out, const notf::_Aabr<VECTOR2>& aabr);

} // namespace notf

// std::hash **********************************************************************************************************/

namespace std {

/** std::hash specialization for notf::_Aabr. */
template <typename VECTOR2>
struct hash<notf::_Aabr<VECTOR2>> {
    size_t operator()(const notf::_Aabr<VECTOR2>& aabr) const { return notf::hash(aabr._min, aabr._max); }
};
}
