#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

#include "notf/meta/exception.hpp"

#include "notf/common/geo/segment.hpp"
#include "notf/common/vector.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// polyline ========================================================================================================= //

/// Baseclass for simple Polylines.
template<class Element>
class Polyline {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type.
    using vector_t = detail::Vector2<Element>;

    /// Element type.
    using element_t = typename vector_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// (empty) Default constructor.
    constexpr Polyline() noexcept = default;

    /// @{
    /// Value constructor.
    /// @param vertices     Vertices from which to construct the Polyline.
    /// @param is_closed    True if the Polyline should be closed even though the last vertex does not match the first.
    Polyline(std::vector<vector_t> vertices, Tristate is_closed = Tristate::Default) noexcept
        : m_vertices(std::move(vertices))
        , m_is_closed((is_closed == Tristate::True)
                      || (m_vertices.empty() ? false : m_vertices.front().is_approx(m_vertices.back()))) {}
    Polyline(std::vector<vector_t> vertices, bool is_closed) noexcept
        : Polyline(std::move(vertices), is_closed ? Tristate::True : Tristate::False) {}
    /// @}

    /// @{
    /// Variadic Constructor taking an arbitrary number of vertices.
    /// @param vertices All vertices that make up the Polyline.
    Polyline(std::initializer_list<vector_t>&& vertices)
        : Polyline(std::vector<vector_t>(std::make_move_iterator(vertices.begin()),
                                         std::make_move_iterator(vertices.end()))) {}
    template<class... Ts, class = std::enable_if_t<all_convertible_to_v<vector_t, Ts...>>>
    Polyline(Ts&&... args) : Polyline({std::forward<Ts>(args)...}) {}
    /// @}

    /// Copy Constructor.
    /// @param other    Polyline to copy from.
    Polyline(const Polyline& other) = default; // required because we have a custom move-constructor

    /// Move Constructor.
    /// @param other    Polyline to move from.
    Polyline(Polyline&& other) : m_vertices(std::move(other.m_vertices)), m_is_closed(other.m_is_closed) {
        other.m_vertices.clear(); // (from reference:) "[other] is left in an unspecified but valid state."
    }

    /// Move operator.
    /// @param other    Polyline to move.
    Polyline& operator=(Polyline&& other) {
        m_vertices = std::move(other.m_vertices);
        other.m_vertices.clear();
        m_is_closed = other.m_is_closed;
        return *this;
    }

    /// Checks whether the Polyline has any vertices or not.
    bool is_empty() const noexcept { return m_vertices.empty(); }
    operator bool() const noexcept { return !is_empty(); }

    /// @{
    /// Vertices of this Polyline.
    std::vector<vector_t>& get_vertices() noexcept { return m_vertices; }
    const std::vector<vector_t>& get_vertices() const noexcept { return m_vertices; }
    /// @}

    /// Returns the number of vertices in this Polyline.
    size_t get_vertex_count() const noexcept { return m_vertices.size(); }

    /// Whether or not this Polyline is closed.
    /// Note that a Polyline with only one or no vertex may still be "closed"
    bool is_closed() const noexcept { return m_is_closed; }

    /// The center point of the Polyline.
    /// If the Polyline is empty, the zero vector is returned.
    vector_t get_center() const noexcept {
        vector_t result = vector_t::zero();
        for (const vector_t& vertex : m_vertices) {
            result += vertex;
        }
        result /= static_cast<element_t>(max(1, m_vertices.size()));
        return result;
    }

    /// The axis-aligned bounding rect of the Polyline.
    /// If the Polyline is empty, the Aabr is invalid.
    Aabr<element_t> get_aabr() const noexcept {
        Aabr<element_t> result = Aabr<element_t>::wrongest();
        for (const vector_t& vertex : m_vertices) {
            result.grow_to(vertex);
        }
        return result;
    }

    /// Calculates the orientation of the Polyline.
    /// @throws ValueError  If the Polyline has no area
    Orientation get_orientation() const {
        if (m_vertices.size() < 3) {
            NOTF_THROW(ValueError, "Cannot get the orientation of a Polyline with zero area");
        }

        // find three consecutive vertices that form a triangle that doesn't contain any other vertices
        Triangle<element_t> triangle;
        bool triangle_is_empty = false;
        for (size_t i = 2; i <= m_vertices.size() && !triangle_is_empty; ++i) {
            triangle = {m_vertices[i - 2], m_vertices[i - 1], m_vertices[i % m_vertices.size()]};
            triangle_is_empty = true;
            for (size_t j = 0; j < m_vertices.size(); ++j) {
                if (triangle.contains(m_vertices[j])) {
                    triangle_is_empty = false;
                    break;
                }
            }
        }
        if (triangle_is_empty) { NOTF_THROW(ValueError, "Cannot get the orientation of a Polyline with zero area"); }

        // the Polyline shares the orientation of the triangle if it is contained
        if (this->contains(triangle.center())) {
            return triangle.orientation();
        } else {
            return (triangle.orientation() == Orientation::CCW) ? Orientation::CW : Orientation::CCW;
        }
    }

    /// Tests if the point is fully contained in the Polyline.
    /// If the point is on the edge of the Polyline, it is not contained within it.
    /// @param point    Point to check.
    bool contains(const vector_t& point) const {
        const Aabr<element_t> aabr = get_aabr();
        if (is_zero(aabr.get_area(), precision_high<element_t>())) { return false; }

        // create a line segment from the point to some point on the outside of the polyline
        const Segment2<element_t> line(point, aabr.get_bottom_left() + vector_t{-1, -1});

        // find the index of the first vertex that does not fall on the line
        size_t index = 0;
        while (index < m_vertices.size() && line.contains(m_vertices[index])) {
            ++index;
        }

        // count the number of intersections with segments of this Polyline
        uint intersections = 0;
        while (index < m_vertices.size()) {
            const vector_t& last_vertex = m_vertices[index++];
            const vector_t& current_vertex = m_vertices[index % m_vertices.size()];
            if (Segment2<element_t>(last_vertex, current_vertex).intersects(line)) { ++intersections; }

            // if the current vertex falls directly onto the line, the line either crosses or touches it
            if (line.contains(current_vertex)) {
                do {
                    ++index; // skip all m_vertices that fall on the line
                } while (line.contains(m_vertices[index % m_vertices.size()]));
                const vector_t& current_vertex = m_vertices[index % m_vertices.size()];

                if (Triangle<element_t>(line.start, line.end, last_vertex).orientation()
                    == Triangle<element_t>(line.start, line.end, current_vertex).orientation()) {
                    // if the last and the current vertex fall on the same side of the line,
                    // the point only touches the Polyline
                    --intersections;
                }
            }
        }

        // the point is contained, if the number of intersections is odd
        return (intersections % 2) == 0;
    }

    /// Checks if this Polyline is convex.
    [[nodiscard]] bool is_convex() const {
        // lines are neither convex or concave, but since a convex Polyline is easier to deal with, we say they are
        if (m_vertices.size() < 3) { return true; }

        // find the first non-zero triangle
        size_t index = 2;
        while (index < m_vertices.size()
               && Triangle<element_t>(m_vertices[0], m_vertices[1], m_vertices[index]).is_degenerate()) {
            ++index;
        }
        const Orientation first_orientation
            = Triangle<element_t>(m_vertices[0], m_vertices[1], m_vertices[index]).get_orientation();

        // check if subsequent triangles always have the same orientation
        for (size_t i = index + 1; i <= m_vertices.size(); ++i) {
            const Triangle<element_t> triangle(m_vertices[i - 2], m_vertices[i - 1], m_vertices[i % m_vertices.size()]);
            if (!triangle.is_degenerate() && triangle.get_orientation() != first_orientation) { return false; }
        }
        return true;
    }

    /// Checks if this Polyline is concave.
    bool is_concave() const { return !is_convex(); }

    /// Equality operator.
    bool operator==(const Polyline& other) const { return m_vertices == other.m_vertices; }

    /// Inequality operator.
    bool operator!=(const Polyline& other) const { return m_vertices != other.m_vertices; }

    /// Tests whether this Polyline is vertex-wise approximate to another.
    /// If you want to know if two Polylines are visually approximate, you'll have to optimize both first.
    /// @param other    Other Polyline to test against.
    /// @param epsilon  Largest ignored difference.
    bool is_approx(const Polyline& other, const element_t epsilon = precision_high<element_t>()) const noexcept {
        if (other.m_vertices.size() != m_vertices.size()) { return false; }
        for (size_t i = 0; i < m_vertices.size(); ++i) {
            if (!m_vertices[i].is_approx(other.m_vertices[i], epsilon)) { return false; }
        }
        return true;
    }

    /// Changes whether this Polyline is closed or not.
    void set_closed(const bool is_closed = true) { m_is_closed = is_closed; }

    /// Remove all vertices that do not add additional corners to the PolyLine.
    void optimize() {
        // merge non-unique vertices (consecutive vertices that share the same position)
        remove_consecutive_equal(m_vertices);

        // do not store an explicit last vertex if the PolyLine is closed anyway
        if (m_is_closed && (m_vertices.empty() ? false : m_vertices.front().is_approx(m_vertices.back()))) {
            m_vertices.pop_back();
        }

        // TODO Polyline::optimize() should remove vertices if they are on the line from last to next as well

        // may conserve some memory
        m_vertices.shrink_to_fit();
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertices of this Polyline.
    std::vector<vector_t> m_vertices;

    /// A closed Polyline has an implicit edge from the last to the first vertex.
    bool m_is_closed = false;
};

} // namespace detail

// conversions ====================================================================================================== //

/// Constructs a rectangular Polyline from an Aabr.
template<>
Polylinef convert_to<Polylinef, Aabrf>(const Aabrf& aabr);

// formatting ======================================================================================================= //

/// Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param polyline  Polyline to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Polylinef& polyline) {
    out << "Polylinef(";
    for (size_t i = 0; i < polyline.get_vertices().size(); ++i) {
        const auto& vertex = polyline.get_vertices()[i];
        out << "(" << vertex.x() << ", " << vertex.y() << ")";
        if (i != polyline.get_vertices().size() - 1) { out << ", "; }
    }
    return out << ")";
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Polyline<Element>> {
    using type = notf::detail::Polyline<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& polyline, FormatContext& ctx) {
        //        return format_to(ctx.begin(), "{}({}, {})", type::get_name(), aabr[0], aabr[1]);
        // TODO: polyline fmt formatting (also print if polyline is closed)
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Triangle.
template<typename REAL>
struct std::hash<notf::detail::Polyline<REAL>> {
    size_t operator()(const notf::detail::Polyline<REAL>& polyline) const {
        auto result = notf::hash(static_cast<size_t>(notf::detail::HashID::POLYLINE));
        for (const auto& vertex : polyline.vertices) {
            notf::hash_combine(result, vertex);
        }
        return result;
    }
};
