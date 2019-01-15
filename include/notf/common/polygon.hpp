#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

#include "notf/meta/exception.hpp"

#include "notf/common/segment.hpp"
#include "notf/common/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// polygon ========================================================================================================== //

/// Baseclass for simple Polygons.
/// "Simple" means that the Polygon has no (non-consecutive) vertices that share the same position, no intersecting
/// edges and must contain at least 3 unique points. Polygons are always closed, meaning the last point is always
/// implicitly connected to the first one.
/// Polygons cannot be modified once they are constructed.
template<class Element>
class Polygon {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vector type.
    using vector_t = detail::Vector2<Element>;

    /// Element type.
    using element_t = typename vector_t::element_t;

    /// Orientation type.
    using Orientation = typename Triangle<element_t>::Orientation;

    // methods --------------------------------------------------------------------------------- //
public:
    /// (empty) Default constructor.
    constexpr Polygon() noexcept = default;

    /// Value constructor.
    /// @param vertices         Vertices from which to construct the Polygon.
    /// @throws runtime_error   If the Polygon does not contain at least 3 unique vertices.
    /// @throws runtime_error   If two non-consecutive vertices share the same position.
    /// @throws runtime_error   If two edges of the Polygon intersect.
    Polygon(std::vector<vector_t> vertices) : m_vertices(_prepare_vertices(std::move(vertices))) {}

    /// Copy constructor.
    Polygon(const Polygon& other) : m_vertices(other.m_vertices) {}

    /// Move constructor.
    Polygon(Polygon&& other) : m_vertices(std::move(other.m_vertices)) {}

    /// Copy operator.
    /// @param other    Polygon to copy.
    Polygon& operator=(const Polygon& other) {
        m_vertices = other.m_vertices;
        return *this;
    }

    /// Move operator.
    /// @param other    Polygon to move.
    Polygon& operator=(Polygon&& other) {
        m_vertices = std::move(other.m_vertices);
        other.m_vertices = {};
        return *this;
    }

    /// Checks whether the Polygon has any vertices or not.
    bool is_empty() const { return m_vertices.empty(); }
    operator bool() const { return !is_empty(); }

    /// Vertices of this Polygon.
    const std::vector<vector_t>& get_vertices() const { return m_vertices; }

    /// Returns the number of vertices in this Polygon.
    size_t get_vertex_count() const { return m_vertices.size(); }

    /// Calculates the orientation of the Polygon.
    Orientation get_orientation() const {
        // find three consecutive vertices that form a triangle that doesn't contain any other vertices
        Triangle<element_t> triangle;
        bool is_empty;
        for (size_t i = 2; i <= m_vertices.size(); ++i) {
            triangle = {m_vertices[i - 2], m_vertices[i - 1], m_vertices[i % m_vertices.size()]};
            is_empty = true;
            for (size_t j = 0; j < m_vertices.size(); ++j) {
                if (triangle.contains(m_vertices[j])) {
                    is_empty = false;
                    break;
                }
            }
            if (is_empty) { break; }
        }
        assert(is_empty);

        // the Polygon shares the orientation of the triangle if it is contained
        if (this->contains(triangle.center())) {
            return triangle.orientation();
        } else {
            return (triangle.orientation() == Orientation::CCW) ? Orientation::CW : Orientation::CCW;
        }
    }

    /// The center point of the Polygon.
    vector_t get_center() const {
        vector_t result = vector_t::zero();
        for (const vector_t& vertex : m_vertices) {
            result += vertex;
        }
        result /= static_cast<element_t>(m_vertices.size());
        return result;
    }

    /// Tests if the point is fully contained in the Polygon.
    /// If the point is on the edge of the Polygon, it is not contained within it.
    /// @param point    Point to check.
    bool contains(const vector_t& point) const {
        // create a line segment from the point to some point on the outside of the polygon
        const Segment2<element_t> line(
            point, vector_t(1, 1)
                       + *(std::max_element(std::begin(m_vertices), std::end(m_vertices),
                                            [](const auto& lhs, const auto& rhs) { return lhs.x() < rhs.x(); })));

        // find the index of the first vertex that does not fall on the line
        size_t index = 0;
        while (index < m_vertices.size() && line.contains(m_vertices[index])) {
            ++index;
        }

        // count the number of intersections with segments of this Polygon
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
                    // the point only touches the Polygon
                    --intersections;
                }
            }
        }

        // the point is contained, if the number of intersections is odd
        return (intersections % 2) == 0;
    }

    /// Checks if this Polygon is convex.
    bool is_convex() const {
        // find the first non-zero triangle
        size_t index = 2;
        while (index < m_vertices.size()
               && Triangle<element_t>(m_vertices[0], m_vertices[1], m_vertices[index]).is_zero()) {
            ++index;
        }
        const Orientation first_orientation
            = Triangle<element_t>(m_vertices[0], m_vertices[1], m_vertices[index]).get_orientation();

        // check if subsequent triangles always have the same orientation
        for (size_t i = index + 1; i <= m_vertices.size(); ++i) {
            const Triangle<element_t> triangle(m_vertices[i - 2], m_vertices[i - 1], m_vertices[i % m_vertices.size()]);
            if (!triangle.is_zero() && triangle.get_orientation() != first_orientation) { return false; }
        }
        return true;
    }

    /// Checks if this Polygon is concave.
    bool is_concave() const { return !is_convex(); }

    /// Tests whether this Polygon is vertex-wise approximate to another.
    /// @param other    Other Polygon to test against.
    /// @param epsilon  Largest ignored difference.
    bool is_approx(const Polygon& other, const element_t epsilon = precision_high<element_t>()) const noexcept {
        if (other.m_vertices.size() != m_vertices.size()) { return false; }
        for (size_t i = 0; i < m_vertices.size(); ++i) {
            if (!m_vertices[i].is_approx(other.m_vertices[i], epsilon)) { return false; }
        }
        return true;
    }

private:
    /// Enforces the construction of a simple Polygon with unique vertices.
    /// @param vertices Vertices from which to construct the Polygon.
    /// @throws LogicError  If the Polygon does not contain at least 3 unique vertices.
    ///                     If two non-consecutive vertices share the same position.
    ///                     If two edges of the Polygon intersect.
    static std::vector<vector_t> _prepare_vertices(std::vector<vector_t>&& vertices) {
        { // merge non-unique vertices (consecutive vertices that share the same position)
            size_t first_non_unique = vertices.size();
            for (size_t i = 1; i < vertices.size(); ++i) {
                if (vertices[i].is_approx(vertices[i - 1])) {
                    first_non_unique = i;
                    break;
                }
            }
            if (first_non_unique != vertices.size()) {
                std::vector<vector_t> unique_vertices;
                unique_vertices.reserve(vertices.size() - 1);

                unique_vertices.insert(std::end(unique_vertices), std::cbegin(vertices),
                                       std::cbegin(vertices) + first_non_unique);

                for (size_t i = first_non_unique + 1; i < vertices.size(); ++i) {
                    if (!vertices[i].is_approx(vertices[i - 1])) { unique_vertices.emplace_back(vertices[i]); }
                }

                std::swap(vertices, unique_vertices);
            }
        }
        vertices.shrink_to_fit();

        if (vertices.size() < 3) { NOTF_THROW(LogicError, "A Polygon must contain at least 3 unique vertices"); }

        for (size_t i = 0; i < vertices.size(); ++i) {
            for (size_t j = i + 1; j < vertices.size(); ++j) {
                if (vertices[i].is_approx(vertices[j])) {
                    NOTF_THROW(LogicError, "Vertices in a Polygon must not share positions");
                }
            }
        }

        // TODO: this seems broken
        //        for (size_t i = 1; i < vertices.size(); ++i) {
        //            for (size_t j = i + 1; j < vertices.size(); ++j) {
        //                if (Segment2<element_t>(vertices[i - 1], vertices[i])
        //                        .intersects(Segment2<element_t>(vertices[j - 1], vertices[j]))) {
        //                    NOTF_THROW(LogicError, "Segments in a Polygon may not intersect");
        //                }
        //            }
        //        }

        return vertices;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertices of this Polygon.
    std::vector<vector_t> m_vertices;
};

} // namespace detail

// conversions ====================================================================================================== //

/// Constructs a rectangular Polygon from an Aabr.
template<>
Polygonf convert_to<Polygonf, Aabrf>(const Aabrf& aabr);

// formatting ======================================================================================================= //

/// Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param polygon  Polygon to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Polygonf& polygon) {
    out << "Polygonf(";
    for (size_t i = 0; i < polygon.get_vertices().size(); ++i) {
        const auto& vertex = polygon.get_vertices()[i];
        out << "(" << vertex.x() << ", " << vertex.y() << ")";
        if (i != polygon.get_vertices().size() - 1) { out << ", "; }
    }
    return out << ")";
}

NOTF_CLOSE_NAMESPACE

namespace fmt {

template<class Element>
struct formatter<notf::detail::Polygon<Element>> {
    using type = notf::detail::Polygon<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& polygon, FormatContext& ctx) {
        //        return format_to(ctx.begin(), "{}({}, {})", type::get_name(), aabr[0], aabr[1]);
        // TODO: polygon fmt formatting
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Triangle.
template<typename REAL>
struct std::hash<notf::detail::Polygon<REAL>> {
    size_t operator()(const notf::detail::Polygon<REAL>& polygon) const {
        auto result = notf::hash(static_cast<size_t>(notf::detail::HashID::POLYGON));
        for (const auto& vertex : polygon.vertices) {
            notf::hash_combine(result, vertex);
        }
        return result;
    }
};
