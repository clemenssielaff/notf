#pragma once

#include "notf/common/geo/size2.hpp"
#include "notf/common/geo/vector2.hpp"

NOTF_OPEN_NAMESPACE

// aabr ============================================================================================================= //

namespace detail {

///  A 2D Axis-Aligned-Bounding-Rectangle.
/// Stores two Vectors, the bottom-left and top-right corner.
/// While this does mean that you need to change four instead of two values for repositioning the Aabr, other
/// calculations (like intersections) are faster; and they are usually more relevant.
template<class Element>
struct Aabr : public Arithmetic<Aabr<Element>, Vector2<Element>, 2> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base class.
    using super_t = Arithmetic<Aabr<Element>, Vector2<Element>, 2>;

public:
    /// Scalar type used by this arithmetic type.
    using element_t = typename super_t::element_t;

    /// Component type used by this arithmetic type.
    using component_t = typename super_t::component_t;

    /// Data holder.
    using Data = typename super_t::Data;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Aabr() noexcept = default;

    /// Value constructor.
    /// @param data Raw data for this arithmetic type.
    constexpr Aabr(Data data) noexcept : super_t(std::move(data)) {}

    /// Constructs an Aabr of the given width and height, with the bottom-left corner at the given coordinates.
    /// @param x         X-coordinate of the bottom-left corner.
    /// @param y         Y-coordinate of the bottom-left corner.
    /// @param width     Width of the Aabr.
    /// @param height    Height of the Aabr.
    constexpr Aabr(const element_t x, const element_t y, const element_t width, const element_t height) noexcept
        : super_t(component_t(x, y), component_t(x + width, y + height)) {}

    /// Constructs an Aabr of the given width and height, with the bottom-left corner at `position`.
    /// @param position  Position of the Aabr's bottom-left corner.
    /// @param width     Width of the Aabr.
    /// @param height    Height of the Aabr.
    constexpr Aabr(const component_t position, const element_t width, const element_t height) noexcept
        : super_t(position, position + component_t(width, height)) {}

    /// Constructs an Aabr of the given size with the bottom-left corner at `position`.
    /// @param position  Position of the Aabr's bottom-left corner.
    /// @param size      Size of the Aabr.
    constexpr Aabr(const component_t& position, const Size2<element_t>& size) noexcept
        : super_t(position, component_t(position.x() + size.get_width(), position.y() + size.get_height())) {}

    /// Constructs an Aabr of the given size with the bottom-left corner at zero.
    /// @param size  Size of the Aabr.
    constexpr Aabr(const Size2<element_t>& size) noexcept
        : super_t(component_t(0, 0), component_t(size.get_width(), size.get_height())) {}

    /// Constructs an Aabr of this element type from any other Aabr type.
    /// @param other    Aabr to copy from.
    template<class T>
    constexpr Aabr(const Aabr<T>& other) noexcept
        : Aabr(other.get_left(), other.get_bottom(), other.get_right(), other.get_top()) {}

    /// Constructs the Aabr from two of its corners.
    /// The corners don't need to be specific, the constructor figures out how to construct an Aabr from them.
    /// @param a    One corner point of the Aabr.
    /// @param b    Opposite corner point of the Aabr.
    constexpr Aabr(const component_t& a, const component_t& b) noexcept {
        if (a.x() < b.x()) {
            if (a.y() < b.y()) {
                data[0] = a;
                data[1] = b;
            } else {
                data[0] = {a.x(), b.y()};
                data[1] = {b.x(), a.y()};
            }
        } else {
            if (a.y() > b.y()) {
                data[0] = b;
                data[1] = a;
            } else {
                data[0] = {b.x(), a.y()};
                data[1] = {a.x(), b.y()};
            }
        }
    }

    /// The largest representable Aabr.
    constexpr static Aabr largest() noexcept {
        Aabr result{};
        result[0] = component_t::all(min_v<element_t>);
        result[1] = component_t::all(max_v<element_t>);
        return result;
    }

    /// The "most wrong" Aabr (maximal negative area).
    /// Is useful as the starting point for defining an Aabr from a set of points.
    constexpr static Aabr wrongest() noexcept {
        Aabr result{};
        result[0] = component_t::all(max_v<element_t>);
        result[1] = component_t::all(min_v<element_t>);
        return result;
    }

    /// Returns an Aabr of a given size, with zero in the center.
    constexpr static Aabr centered(const Size2<element_t>& size) noexcept {
        const element_t half_width = size.get_width() / 2;
        const element_t half_height = size.get_height() / 2;
        Aabr result{};
        result.data[0] = {-half_width, -half_height};
        result.data[1] = {half_width, half_height};
        return result;
    }

    /// Name of this Aabr type.
    static constexpr const char* get_type_name() { // TODO: this could be a free function used by all mathematic classes
        if constexpr (std::is_same_v<Element, float>) {
            return "Aabrf";
        } else if constexpr (std::is_same_v<Element, double>) {
            return "Aabrd";
        } else if constexpr (std::is_same_v<Element, int>) {
            return "Aabri";
        } else if constexpr (std::is_same_v<Element, short>) {
            return "Aabrs";
        }
    }

    /// X-coordinate of the left edge of this Aabr.
    constexpr const element_t& get_left() const noexcept { return data[0].x(); }

    /// Sets the x-coordinate of this Aabr's left edge.
    /// If the new position is further right than the Aabr's right edge, the right edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    constexpr Aabr& set_left(const element_t x) noexcept {
        _left() = x;
        _right() = get_right() > get_left() ? get_right() : get_left();
        return *this;
    }

    /// X-coordinate of the right edge of this Aabr.
    constexpr const element_t& get_right() const noexcept { return data[1].x(); }

    /// Sets the x-coordinate of this Aabr's right edge.
    /// If the new position is further left than the Aabr's left edge, the left edge is moved to the same
    /// position, resulting in a Aabr with zero width.
    constexpr Aabr& set_right(const element_t x) noexcept {
        _right() = x;
        _left() = get_left() < get_right() ? get_left() : get_right();
        return *this;
    }

    /// Y-coordinate of the top edge of this Aabr.
    constexpr const element_t& get_top() const noexcept { return data[1].y(); }

    /// Sets the y-coordinate of this Aabr's top edge.
    /// If the new position is further down than the Aabr's bottom edge, the bottom edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    constexpr Aabr& set_top(const element_t y) noexcept {
        _top() = y;
        _bottom() = get_bottom() < get_top() ? get_bottom() : get_top();
        return *this;
    }

    /// Y-coordinate of the bottom edge of this Aabr.
    constexpr const element_t& get_bottom() const noexcept { return data[0].y(); }

    /// Sets the y-coordinate of this Aabr's bottom edge.
    /// If the new position is further up than the Aabr's top edge, the top edge is moved to the same
    /// position, resulting in a Aabr with zero height.
    constexpr Aabr& set_bottom(const element_t y) noexcept {
        _bottom() = y;
        _top() = get_top() > get_bottom() ? get_top() : get_bottom();
        return *this;
    }

    /// The center of the Aabr.
    constexpr component_t get_center() const noexcept { return (data[0] + data[1]) / 2; }

    /// Moves this Aabr to a new center position.
    constexpr Aabr& set_center(const component_t& pos) noexcept {
        set_center_x(pos.x());
        return set_center_y(pos.y());
    }

    /// The horizontal center of the Aabr.
    constexpr element_t get_center_x() const noexcept { return (data[0].x() + data[1].x()) / 2; }

    /// Moves the center of this Aabr to the given x-coordinate.
    constexpr Aabr& set_center_x(const element_t x) noexcept {
        const element_t half_width = get_width() / 2;
        _left() = x - half_width;
        _right() = x + half_width;
        return *this;
    }

    /// The vertical center of the Aabr.
    constexpr element_t get_center_y() const noexcept { return (data[0].y() + data[1].y()) / 2; }

    /// Moves the center of this Aabr to the given y-coordinate.
    constexpr Aabr& set_center_y(const element_t y) noexcept {
        const element_t half_height = get_height() / 2;
        _bottom() = y - half_height;
        _top() = y + half_height;
        return *this;
    }

    /// The bottom left corner of this Aabr.
    constexpr const component_t& get_bottom_left() const noexcept { return data[0]; }

    /// Sets a new bottom-left corner of this Aabr.
    /// See set_left and set_bottom for details.
    constexpr Aabr& set_bottom_left(const component_t& point) noexcept {
        set_left(point.x());
        return set_bottom(point.y());
    }

    /// The top right corner of this Aabr.
    constexpr const component_t& get_top_right() const noexcept { return data[1]; }

    /// Sets a new top-right corner of this Aabr.
    /// See set_right and set_top for details.
    constexpr Aabr& set_top_right(const component_t& point) noexcept {
        set_right(point.x());
        return set_top(point.y());
    }

    /// The top left corner of this Aabr.
    constexpr component_t get_top_left() const noexcept { return {get_left(), get_top()}; }

    /// Sets a new top-left corner of this Aabr.
    /// See set_left and set_top for details.
    constexpr Aabr& set_top_left(const component_t& point) noexcept {
        set_left(point.x());
        return set_top(point.y());
    }

    /// The bottom right corner of this Aabr.
    constexpr component_t get_bottom_right() const noexcept { return {get_right(), get_bottom()}; }

    /// Sets a new bottom-right corner of this Aabr.
    /// See set_right and set_bottom for details.
    constexpr Aabr& set_bottom_right(const component_t& point) noexcept {
        set_right(point.x());
        return set_bottom(point.y());
    }

    /// The width of this Aabr.
    constexpr element_t get_width() const noexcept { return get_right() - get_left(); }

    /// Changes the height of this Aabr in place.
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a width less than zero is specified, the resulting width is zero.
    constexpr Aabr& set_width(const element_t width) noexcept {
        const element_t center = get_center_x();
        const element_t half_width = max(0, width / 2);
        _left() = center - half_width;
        _right() = center + half_width;
        return *this;
    }

    /// The height of this Aabr.
    constexpr element_t get_height() const noexcept { return get_top() - get_bottom(); }

    /// Changes the height of this Aabr in place.
    /// The scaling occurs from the center of the Aabr, meaning its position does not change.
    /// If a height less than zero is specified, the resulting height is zero.
    constexpr Aabr& set_height(const element_t height) noexcept {
        const element_t center = get_center_y();
        const element_t half_height = max(0, height / 2);
        _bottom() = center - half_height;
        _top() = center + half_height;
        return *this;
    }

    /// Returns the extend of this Aabr.
    constexpr Size2<element_t> get_size() const noexcept { return {get_width(), get_height()}; }

    /// Returns the extend of this Aabr.
    constexpr Aabr& set_size(const Size2<element_t>& size) noexcept {
        const component_t center = get_center();
        const Size2<element_t> half_size = size / 2;
        _left() = center.x() - half_size.get_width();
        _right() = center.x() + half_size.get_width();
        _bottom() = center.x() - half_size.get_height();
        _top() = center.x() + half_size.get_height();
        return *this;
    }

    /// The area of this Aabr.
    constexpr element_t get_area() const noexcept { return get_height() * get_width(); }

    /// A valid Aabr has a width and height >= 0.
    constexpr bool is_valid() const noexcept { return get_left() <= get_right() && get_bottom() <= get_top(); }

    /// Checks if this Aabr contains a given point.
    constexpr bool contains(const component_t& point) const noexcept {
        return point.x() > get_left() &&   //
               point.x() < get_right() &&  //
               point.y() > get_bottom() && //
               point.y() < get_top();
    }

    /// Checks if two Aabrs intersect.
    /// Two Aabrs intersect if their intersection has a non-zero area.
    /// To get the actual intersection Aabr, use Aabr::intersection().
    constexpr bool intersects(const Aabr& other) const noexcept {
        return !(get_right() < other.get_left() || //
                 get_left() > other.get_right() || //
                 get_bottom() > other.get_top() || //
                 get_top() < other.get_bottom());
    }

    /// Returns the closest point inside the Aabr to a given target point.
    /// For targets outside the Aabr, the returned point will lay on the Aabr's edge.
    /// Targets inside the Aabr are returned unchanged.
    /// @param target    Target point.
    /// @return          Closest point inside the Aabr to the target point.
    constexpr component_t get_closest_point_to(const component_t& target) const noexcept {
        const component_t pos = get_center();
        const element_t half_width = get_width() / 2;
        const element_t half_height = get_height() / 2;
        return {pos.x() + clamp(target.x() - pos.x(), -half_width, half_width),
                pos.y() + clamp(target.y() - pos.y(), -half_height, half_height)};
    }

    /// Returns the length of the longer side of this Aabr.
    constexpr element_t get_longer_side() const noexcept {
        const element_t width = get_width();
        const element_t height = get_height();
        return width > height ? width : height;
    }

    /// Returns the length of the shorter side of this Aabr.
    constexpr element_t get_shorter_side() const noexcept {
        const element_t width = get_width();
        const element_t height = get_height();
        return width < height ? width : height;
    }

    /// Moves this Aabr by a relative amount.
    constexpr Aabr& move_by(const component_t& pos) noexcept {
        data[0] += pos;
        data[1] += pos;
        return *this;
    }

    /// Moves each edge of the Aabr a given amount towards the outside.
    /// Meaning, the width/height of the Aabr will grow by 2*amount.
    /// @param amount    Number of units to move each edge.
    constexpr Aabr& grow(element_t amount) noexcept {
        _left() -= amount;
        _bottom() -= amount;
        _right() += amount;
        _top() += amount;
        if (get_left() > get_right()) { _left() = _right() = get_center_x(); }
        if (get_bottom() > get_top()) { _bottom() = _top() = get_center_y(); }
        return *this;
    }

    /// Returns a grown copy of this Aabr.
    constexpr Aabr get_grown(const element_t amount) const& noexcept {
        Aabr result(*this);
        result.grow(amount);
        return result;
    }
    constexpr Aabr& get_grown(const element_t amount) && noexcept { return grow(amount); }

    /// Grows this Aabr to include the given point.
    /// If the point is already within the rectangle, this does nothing.
    /// @param point     Point to include in the Aabr.
    constexpr Aabr& grow_to(const component_t& point) noexcept {
        _left() = min(get_left(), point.x());
        _bottom() = min(get_bottom(), point.y());
        _right() = max(get_right(), point.x());
        _top() = max(get_top(), point.y());
        return *this;
    }

    /// Moves each edge of the Aabr a given amount towards the inside.
    ///  Meaning, the width/height of the Aabr will shrink by 2*amount.
    /// You cannot shrink the Aabr to negative width or height values.
    /// @param amount    Number of units to move each edge.
    constexpr Aabr& shrink(const element_t amount) noexcept { return grow(-amount); }

    /// @{
    /// Returns a shrunken copy of this Aabr.
    constexpr Aabr get_shrunken(const element_t amount) const& noexcept {
        Aabr result(*this);
        result.shrink(amount);
        return result;
    }
    constexpr Aabr& get_shrunken(const element_t amount) && noexcept { return shrink(amount); }

    /// @{
    /// Intersection of this Aabr with `other` in-place.
    /// Intersecting with another Aabr that does not intersect results in the zero Aabr.
    constexpr Aabr& intersect(const Aabr& other) noexcept {
        if (!intersects(other)) { return super_t::set_all(0); }
        _left() = get_left() > other.get_left() ? get_left() : other.get_left();
        _right() = get_right() < other.get_right() ? get_right() : other.get_right();
        _bottom() = get_bottom() > other.get_bottom() ? get_bottom() : other.get_bottom();
        _top() = get_top() < other.get_top() ? get_top() : other.get_top();
        return *this;
    }
    constexpr Aabr operator&=(const Aabr& other) noexcept { return intersect(other); }

    /// @{
    /// Intersection of this Aabr with `other`.
    /// Intersecting with another Aabr that does not intersect results in the zero Aabr.
    /// @return  The intersection Aabr.
    constexpr Aabr get_intersection(const Aabr& other) const& noexcept {
        if (!intersects(other)) { return Aabr::zero(); }
        return Aabr(component_t{get_left() > other.get_left() ? get_left() : other.get_left(),
                                get_bottom() > other.get_bottom() ? get_bottom() : other.get_bottom()},
                    component_t{get_right() < other.get_right() ? get_right() : other.get_right(),
                                get_top() < other.get_top() ? get_top() : other.get_top()});
    }
    constexpr Aabr& get_intersection(const Aabr& other) && noexcept { return intersect(other); }
    constexpr Aabr operator&(const Aabr& other) const& noexcept { return get_intersection(other); }
    constexpr Aabr& operator&(const Aabr& other) && noexcept { return intersect(other); }

    /// @{
    /// Creates the union of this Aabr with `other` in-place.
    Aabr& unite(const Aabr& other) {
        _left() = get_left() < other.get_left() ? get_left() : other.get_left();
        _bottom() = get_bottom() < other.get_bottom() ? get_bottom() : other.get_bottom();
        _right() = get_right() > other.get_right() ? get_right() : other.get_right();
        _top() = get_top() > other.get_top() ? get_top() : other.get_top();
        return *this;
    }
    Aabr& operator|=(const Aabr& other) { return unite(other); }

    /// @{
    /// Creates the union of this Aabr with `other`.
    Aabr get_union(const Aabr& other) const& {
        return Aabr(component_t{get_left() < other.get_left() ? get_left() : other.get_left(),
                                get_bottom() < other.get_bottom() ? get_bottom() : other.get_bottom()},
                    component_t{get_right() > other.get_right() ? get_right() : other.get_right(),
                                get_top() > other.get_top() ? get_top() : other.get_top()});
    }
    Aabr& get_union(const Aabr& other) && { return unite(other); }
    Aabr operator|(const Aabr& other) const& { return get_union(other); }
    Aabr& operator|(const Aabr& other) && { return unite(other); }

private:
    constexpr element_t& _left() noexcept { return data[0].x(); }
    constexpr element_t& _right() noexcept { return data[1].x(); }
    constexpr element_t& _top() noexcept { return data[1].y(); }
    constexpr element_t& _bottom() noexcept { return data[0].y(); }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    using super_t::data; // TODO: this shouldn't be public
};

} // namespace detail

// formatting ======================================================================================================= //

/// Prints the contents of an Aabr into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param aabr Aabr to print.
/// @return Output stream for further output.
template<class Element>
std::ostream& operator<<(std::ostream& out, const notf::detail::Aabr<Element>& aabr) {
    return out << fmt::format("{}", aabr);
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Aabr<Element>> {
    using type = notf::detail::Aabr<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& aabr, FormatContext& ctx) {
        return format_to(ctx.begin(), "{}({}, {})", type::get_type_name(), aabr[0], aabr[1]);
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Aabr.
template<class Element>
struct std::hash<notf::detail::Aabr<Element>> {
    size_t operator()(const notf::detail::Aabr<Element>& aabr) const {
        return notf::hash(notf::to_number(notf::detail::HashID::AABR), aabr.get_hash());
    }
};

// compile time tests =============================================================================================== //

static_assert(notf::is_pod_v<notf::Aabrf>);
static_assert(notf::is_pod_v<notf::Aabrd>);
static_assert(notf::is_pod_v<notf::Aabri>);
static_assert(notf::is_pod_v<notf::Aabrs>);

static_assert(sizeof(notf::Aabrf) == sizeof(float) * 4);
static_assert(sizeof(notf::Aabrd) == sizeof(double) * 4);
static_assert(sizeof(notf::Aabri) == sizeof(int) * 4);
static_assert(sizeof(notf::Aabrs) == sizeof(short) * 4);
