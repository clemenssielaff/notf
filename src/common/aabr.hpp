#pragma once

#include <iosfwd>
#include <limits>

#include "common/float.hpp"
#include "common/hash.hpp"
#include "common/matrix3.hpp"
#include "common/matrix4.hpp"
#include "common/meta.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

// TODO: AABR is used everywhere - why does it include sooo many other includes?

NOTF_OPEN_NAMESPACE

namespace detail {

// ================================================================================================================== //

///  A 2D Axis-Aligned-Bounding-Rectangle.
/// Stores two Vectors, the bottom-left and top-right corner.
/// While this does mean that you need to change four instead of two values for repositioning the Aabr, other
/// calculations (like intersections) are faster; and they are usually more relevant.
template<typename VECTOR2>
struct Aabr {

    /// Vector type.
    using vector_t = VECTOR2;

    /// Element type.
    using element_t = typename VECTOR2::element_t;

    // fields ------------------------------------------------------------------------------------------------------- //
    /// Bottom-left corner of the Aabr.
    vector_t _min;

    /// Top-right corner of the Aabr.
    vector_t _max;

    // methods ------------------------------------------------------------------------------------------------------ //
    /// Default constructor.
    Aabr() = default;

    /// Constructs an Aabr of the given width and height, with the bottom-left corner at the given coordinates.
    /// @param x         X-coordinate of the bottom-left corner.
    /// @param y         Y-coordinate of the bottom-left corner.
    /// @param width     Width of the Aabr.
    /// @param height    Height of the Aabr.
    template<typename X, typename Y, typename WIDTH, typename HEIGHT>
    Aabr(const X x, const Y y, const WIDTH width, const HEIGHT height)
        : _min(static_cast<element_t>(x), static_cast<element_t>(y))
        , _max(static_cast<element_t>(x) + static_cast<element_t>(width),
               static_cast<element_t>(y) + static_cast<element_t>(height))
    {}

    /// Constructs an Aabr of the given width and height, with the bottom-left corner at `position`.
    /// @param position  Position of the Aabr's bottom-left corner.
    /// @param width     Width of the Aabr.
    /// @param height    Height of the Aabr.
    template<typename POSITION, typename WIDTH, typename HEIGHT>
    Aabr(const POSITION& position, const WIDTH width, const HEIGHT height)
        : _min(static_cast<vector_t>(position))
        , _max(static_cast<vector_t>(position).x() + static_cast<element_t>(width),
               static_cast<vector_t>(position).y() + static_cast<element_t>(height))
    {}

    /// Constructs an Aabr of the given size with the bottom-left corner at `position`.
    /// @param position  Position of the Aabr's bottom-left corner.
    /// @param size      Size of the Aabr.
    Aabr(const vector_t& position, const Size2f& size)
        : _min(position), _max(position.x() + size.width, position.y() + size.height)
    {}

    /// Constructs an Aabr of the given size with the bottom-left corner at zero.
    /// @param size  Size of the Aabr.
    Aabr(const Size2<element_t>& size) : _min(0, 0), _max(size.width, size.height) {}

    /// Constructs the Aabr from two of its corners.
    /// The corners don't need to be specific, the constructor figures out how to construct an Aabr from them.
    /// @param a    One corner point of the Aabr.
    /// @param b    Opposite corner point of the Aabr.
    Aabr(const vector_t& a, const vector_t& b)
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

    /// The zero Aabr.
    static Aabr zero() { return Aabr({0, 0, 0, 0}); }

    /// The largest representable Aabr.
    static Aabr huge()
    {
        Aabr result;
        result._min = vector_t::fill(min_value<element_t>());
        result._max = vector_t::fill(max_value<element_t>());
        return result;
    }

    /// The "most wrong" Aabr (maximal negative area).
    /// Is useful as the starting point for defining an Aabr from a set of points.
    static Aabr wrongest()
    {
        Aabr result;
        result._min = vector_t::fill(max_value<element_t>());
        result._max = vector_t::fill(min_value<element_t>());
        return result;
    }

    /// Returns an Aabr of a given size, with zero in the center.
    static Aabr centered(const Size2<element_t>& size)
    {
        const element_t half_width = size.width / 2;
        const element_t half_height = size.height / 2;
        Aabr result;
        result._min = {-half_width, -half_height};
        result._max = {half_width, half_height};
        return result;
    }

    /// Creates a copy of this Aabr.
    Aabr get_copy() const { return *this; }

    /// X-coordinate of the center point.
    element_t get_x() const { return (_min.x() + _max.x()) / 2; }

    /// Y-coordinate of the center point.
    element_t get_y() const { return (_min.y() + _max.y()) / 2; }

    /// The center of the Aabr.
    vector_t get_center() const { return {get_x(), get_y()}; }

    /// X-coordinate of the left edge of this Aabr.
    element_t get_left() const { return _min.x(); }

    /// X-coordinate of the right edge of this Aabr.
    element_t get_right() const { return _max.x(); }

    /// Y-coordinate of the top edge of this Aabr.
    element_t get_top() const { return _max.y(); }

    /// Y-coordinate of the bottom edge of this Aabr.
    element_t get_bottom() const { return _min.y(); }

    /// The bottom left corner of this Aabr.
    const vector_t& get_bottom_left() const { return _min; }

    /// The top right corner of this Aabr.
    const vector_t& get_top_right() const { return _max; }

    /// The top left corner of this Aabr.
    vector_t get_top_left() const { return {_min.x(), _max.y()}; }

    /// The bottom right corner of this Aabr.
    vector_t get_bottom_right() const { return {_max.x(), _min.y()}; }

    /// The width of this Aabr.
    element_t get_width() const { return _max.x() - _min.x(); }

    /// The height of this Aabr.
    element_t get_height() const { return _max.y() - _min.y(); }

    /// The area of this Aabr.
    element_t get_area() const { return get_height() * get_width(); }

    /// A valid Aabr has a width and height >= 0.
    bool is_valid() const { return _min.x() <= _max.x() && _min.y() <= _max.y(); }

    /// Test, if this Aabr is zero.
    /// The zero Aabr has no area and is located at zero.
    bool is_zero() const { return _min.is_zero() && _max.is_zero(); }

    /// Checks if this Aabr contains a given point.
    bool contains(const vector_t& point) const
    {
        return ((point.x() > _min.x()) && (point.x() < _max.x()) && (point.y() > _min.y()) && (point.y() < _max.y()));
    }

    /// Checks if two Aabrs intersect.
    /// Two Aabrs intersect if their intersection has a non-zero area.
    /// To get the actual intersection Aabr, use Aabr::intersection().
    bool intersects(const Aabr& other) const
    {
        return !((_max.x() < other._min.x()) || (_min.x() > other._max.x()) || (_min.y() > other._max.y())
                 || (_max.y() < other._min.y()));
    }

    /// Returns the closest point inside the Aabr to a given target point.
    /// For targets outside the Aabr, the returned point will lay on the Aabr's edge.
    /// Targets inside the Aabr are returned unchanged.
    /// @param target    Target point.
    /// @return          Closest point inside the Aabr to the target point.
    vector_t get_closest_point_to(const vector_t& target) const
    {
        const vector_t pos = get_center();
        const element_t half_width = get_width() / 2;
        const element_t half_height = get_height() / 2;
        return {pos.x() + clamp(target.x() - pos.x(), -half_width, half_width),
                pos.y() + clamp(target.y() - pos.y(), -half_height, half_height)};
    }

    /// Returns the extend of this Aabr.
    Size2<element_t> get_size() const { return {get_width(), get_height()}; }

    /// Returns the length of the longer side of this Aabr.
    element_t get_longer_side() const
    {
        const element_t width = get_width();
        const element_t height = get_height();
        return width > height ? width : height;
    }

    /// Returns the length of the shorter side of this Aabr.
    element_t get_shorter_side() const
    {
        const element_t width = get_width();
        const element_t height = get_height();
        return width < height ? width : height;
    }

    /// Read-only pointer to the Aabr's internal storage.
    const element_t* as_ptr() const { return &_min.x(); }

    /// Tests whether two Aabrs are equal.
    bool operator==(const Aabr& other) const { return (other._min == _min && other._max == _max); }

    /// Tests whether two Aabrs are not equal.
    bool operator!=(const Aabr& other) const { return (other._min != _min || other._max != _max); }

    /// Multiplication with a scalar scales the Aabr in relation to local zero.
    Aabr operator*(const element_t factor) { return {_min * factor, _max * factor}; }

    /// Adding a vector_t to an Aabr moves the Aabr relative to its previous position.
    Aabr operator+(const vector_t& offset) { return {_min + offset, _max + offset}; }

    /// Moves the center of this Aabr to the given x-coordinate.
    Aabr& set_x(const element_t x)
    {
        const element_t half_width = get_width() / 2;
        _min.x() = x - half_width;
        _max.x() = x + half_width;
        return *this;
    }

    /// Moves the center of this Aabr to the given y-coordinate.
    Aabr& set_y(const element_t y)
    {
        const element_t half_height = get_height() / 2;
        _min.y() = y - half_height;
        _max.y() = y + half_height;
        return *this;
    }

    /// Moves this Aabr to a new center position.
    Aabr& set_center(const vector_t& pos)
    {
        set_x(pos.x());
        return set_y(pos.y());
    }

    /// Moves this Aabr by a relative amount.
    Aabr& move_by(const vector_t& pos)
    {
        _min += pos;
        _max += pos;
        return *this;
    }

    /// Sets the x-coordinate of this Aabr's left edge.
    /// If the new position is further right than the Aabr's right edge, the right edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    Aabr& set_left(const element_t x)
    {
        _min.x() = x;
        _max.x() = _max.x() > _min.x() ? _max.x() : _min.x();
        return *this;
    }

    /// Sets the x-coordinate of this Aabr's right edge.
    /// If the new position is further left than the Aabr's left edge, the left edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    Aabr& set_right(const element_t x)
    {
        _max.x() = x;
        _min.x() = _min.x() < _max.x() ? _min.x() : _max.x();
        return *this;
    }

    /// Sets the y-coordinate of this Aabr's top edge.
    /// If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    Aabr& set_top(const element_t y)
    {
        _max.y() = y;
        _min.y() = _min.y() < _max.y() ? _min.y() : _max.y();
        return *this;
    }

    /// Sets the y-coordinate of this Aabr's bottom edge.
    /// If the new position is further up than the Aabr's top edge, the top edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    Aabr& set_bottom(const element_t y)
    {
        _min.y() = y;
        _max.y() = _max.y() > _min.y() ? _max.y() : _min.y();
        return *this;
    }

    /// Sets a new top-left corner of this Aabr.
    /// See set_left and set_top for details.
    Aabr& set_top_left(const vector_t& point)
    {
        set_left(point.x());
        return set_top(point.y());
    }

    /// Sets a new top-right corner of this Aabr.
    /// See set_right and set_top for details.
    Aabr& set_top_right(const vector_t& point)
    {
        set_right(point.x());
        return set_top(point.y());
    }

    /// Sets a new bottom-left corner of this Aabr.
    /// See set_left and set_bottom for details.
    Aabr& set_bottom_left(const vector_t& point)
    {
        set_left(point.x());
        return set_bottom(point.y());
    }

    /// Sets a new bottom-right corner of this Aabr.
    /// See set_right and set_bottom for details.
    Aabr& set_bottom_right(const vector_t& point)
    {
        set_right(point.x());
        return set_bottom(point.y());
    }

    /// Changes the height of this Aabr in place.
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a width less than zero is specified, the resulting width is zero.
    Aabr& set_width(const element_t width)
    {
        const element_t center = get_x();
        const element_t half_width = width / 2;
        _min.x() = center - half_width;
        _max.x() = center + half_width;
        return *this;
    }

    /// Changes the height of this Aabr in place.
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a height less than zero is specified, the resulting height is zero.
    Aabr& set_height(const element_t height)
    {
        const element_t center = get_y();
        const element_t half_height = height / 2;
        _min.y() = center - half_height;
        _max.y() = center + half_height;
        return *this;
    }

    /// Sets this Aabr to zero.
    Aabr& set_zero()
    {
        _min.set_zero();
        _max.set_zero();
        return *this;
    }

    /// Moves each edge of the Aabr a given amount towards the outside.
    /// Meaning, the width/height of the Aabr will grow by 2*amount.
    /// @param amount    Number of units to move each edge.
    Aabr& grow(const element_t amount)
    {
        _min.x() -= amount;
        _min.y() -= amount;
        _max.x() += amount;
        _max.y() += amount;
        return *this;
    }

    /// Returns a grown copy of this Aabr.
    Aabr grow(const element_t amount) const
    {
        Aabr result(*this);
        result.grow(amount);
        return result;
    }

    /// Grows this Aabr to include the given point.
    /// If the point is already within the rectangle, this does nothing.
    /// @param point     Point to include in the Aabr.
    Aabr& grow_to(const vector_t& point)
    {
        _min.x() = min(_min.x(), point.x());
        _min.y() = min(_min.y(), point.y());
        _max.x() = max(_max.x(), point.x());
        _max.y() = max(_max.y(), point.y());
        return *this;
    }

    /// Moves each edge of the Aabr a given amount towards the inside.
    ///  Meaning, the width/height of the Aabr will shrink by 2*amount.
    /// You cannot shrink the Aabr to negative width or height values.
    /// @param amount    Number of units to move each edge.
    Aabr& shrink(const element_t amount) { return grow(-amount); }

    /// Returns a shrunken copy of this Aabr.
    Aabr shrink(const element_t amount) const
    {
        Aabr result(*this);
        result.shrink(amount);
        return result;
    }

    /// Intersection of this Aabr with `other`.
    /// Intersecting with another Aabr that does not intersect results in the zero Aabr.
    /// @return  The intersection Aabr.
    Aabr get_intersection(const Aabr& other) const
    {
        if (!intersects(other)) {
            return Aabr::zero();
        }
        return Aabr(vector_t{_min.x() > other._min.x() ? _min.x() : other._min.x(),
                             _min.y() > other._min.y() ? _min.y() : other._min.y()},
                    vector_t{_max.x() < other._max.x() ? _max.x() : other._max.x(),
                             _max.y() < other._max.y() ? _max.y() : other._max.y()});
    }
    Aabr operator&(const Aabr& other) const { return get_intersection(other); }

    /// Intersection of this Aabr with `other` in-place.
    /// Intersecting with another Aabr that does not intersect results in the zero Aabr.
    Aabr& intersect(const Aabr& other)
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
    Aabr operator&=(const Aabr& other) { return intersect(other); }

    /// Creates the union of this Aabr with `other`.
    Aabr get_union(const Aabr& other) const
    {
        return Aabr(vector_t{_min.x() < other._min.x() ? _min.x() : other._min.x(),
                             _min.y() < other._min.y() ? _min.y() : other._min.y()},
                    vector_t{_max.x() > other._max.x() ? _max.x() : other._max.x(),
                             _max.y() > other._max.y() ? _max.y() : other._max.y()});
    }
    Aabr operator|(const Aabr& other) const { return get_union(other); }

    /// Creates the union of this Aabr with `other` in-place.
    Aabr& unite(const Aabr& other)
    {
        _min.x() = _min.x() < other._min.x() ? _min.x() : other._min.x();
        _min.y() = _min.y() < other._min.y() ? _min.y() : other._min.y();
        _max.x() = _max.x() > other._max.x() ? _max.x() : other._max.x();
        _max.y() = _max.y() > other._max.y() ? _max.y() : other._max.y();
        return *this;
    }
    Aabr& operator|=(const Aabr& other) { return unite(other); }

    /// Read-write pointer to the Aabr's internal storage.
    element_t* as_ptr() { return &_min.x(); }

    /// Applies a transformation to this Aabr in-place.
    template<typename MATRIX>
    Aabr& transformed_by(const MATRIX& matrix)
    {
        typename MATRIX::component_t d0(_min[0], _min[1]);
        typename MATRIX::component_t d1(_max[0], _max[1]);
        typename MATRIX::component_t d2(_min.x(), _max.y());
        typename MATRIX::component_t d3(_max.x(), _min.y());
        d0 = matrix.transform(d0);
        d1 = matrix.transform(d1);
        d2 = matrix.transform(d2);
        d3 = matrix.transform(d3);
        _min.x() = min(d0.x(), d1.x(), d2.x(), d3.x());
        _min.y() = min(d0.y(), d1.y(), d2.y(), d3.y());
        _max.x() = max(d0.x(), d1.x(), d2.x(), d3.x());
        _max.y() = max(d0.y(), d1.y(), d2.y(), d3.y());
        return *this;
    }
};

} // namespace detail

// ================================================================================================================== //

using Aabrf = detail::Aabr<Vector2f>;
using Aabrd = detail::Aabr<Vector2d>;
using Aabri = detail::Aabr<Vector2i>;
using Aabrs = detail::Aabr<Vector2s>;

// ================================================================================================================== //

namespace detail {

template<>
inline Aabrf matrix3_transform(const Matrix3f& xform, const Aabrf& aabr)
{
    return aabr.get_copy().transformed_by(xform);
}

template<>
inline Aabrd matrix3_transform(const Matrix3d& xform, const Aabrd& aabr)
{
    return aabr.get_copy().transformed_by(xform);
}

template<>
inline Aabrf transform3(const Matrix4f& xform, const Aabrf& aabr)
{
    return aabr.get_copy().transformed_by(xform);
}

template<>
inline Aabrd transform3(const Matrix4d& xform, const Aabrd& aabr)
{
    return aabr.get_copy().transformed_by(xform);
}

} // namespace detail

// ================================================================================================================== //

/// Prints the contents of an Aabr into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param aabr AABR to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Aabrf& aabr);
std::ostream& operator<<(std::ostream& out, const Aabrd& aabr);
std::ostream& operator<<(std::ostream& out, const Aabri& aabr);

NOTF_CLOSE_NAMESPACE

//== std::hash ====================================================================================================== //

namespace std {

/// std::hash specialization for Aabr.
template<typename VECTOR2>
struct hash<notf::detail::Aabr<VECTOR2>> {
    size_t operator()(const notf::detail::Aabr<VECTOR2>& aabr) const
    {
        return notf::hash(static_cast<size_t>(notf::detail::HashID::AABR), aabr._min, aabr._max);
    }
};
} // namespace std
