#pragma once

#include <iostream>

#include "notf/common/geo/segment.hpp"
#include "notf/common/vector.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

// TODO: create polyline class and split it from polygon?

// polygon2 ========================================================================================================= //

/// Baseclass for a simple 2D Polygon.
/// As defined in https://en.wikipedia.org/wiki/Simple_polygon
template<class Element>
class Polygon2 {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Vertex type.
    using vertex_t = detail::Vector2<Element>;

    /// Element type.
    using element_t = typename vertex_t::element_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// (empty) Default constructor.
    constexpr Polygon2() noexcept = default;

    /// Value constructor.
    /// @param vertices     Vertices from which to construct the Polygon.
    explicit Polygon2(std::vector<vertex_t> vertices) noexcept : m_vertices(std::move(vertices)) {}

    /// Checks whether the Polygon has any vertices.
    constexpr bool is_empty() const noexcept { return m_vertices.empty(); }
    constexpr explicit operator bool() const noexcept { return !is_empty(); }

    /// @{
    /// The vertices of this Polygon.
    constexpr std::vector<vertex_t>& get_vertices() noexcept { return m_vertices; }
    constexpr const std::vector<vertex_t>& get_vertices() const noexcept { return m_vertices; }
    /// @}

    /// @{
    /// The center point of the Polygon.
    /// If the Polygon is empty, the zero vector is returned.
    static constexpr vertex_t get_center(const std::vector<vertex_t>& vertices) noexcept {
        vertex_t result = vertex_t::zero();
        for (const vertex_t& vertex : vertices) {
            result += vertex;
        }
        result /= static_cast<element_t>(max(1, vertices.size()));
        return result;
    }
    constexpr vertex_t get_center() const noexcept { return get_center(m_vertices); }
    /// @}

    /// @{
    /// The axis-aligned bounding rect of the Polygon.
    /// If the Polygon is empty, the Aabr is invalid.
    static constexpr Aabr<element_t> get_aabr(const std::vector<vertex_t>& vertices) noexcept {
        Aabr<element_t> result = Aabr<element_t>::wrongest();
        for (const vertex_t& vertex : vertices) {
            result.grow_to(vertex);
        }
        return result;
    }
    constexpr Aabr<element_t> get_aabr() const noexcept { return get_aabr(m_vertices); }
    /// @}

    /// @{
    /// Checks if this Polygon is convex.
    static bool is_convex(const std::vector<vertex_t>& vertices) noexcept {
        // lines are neither convex or concave, but since a convex Polygon is easier to deal with, we say they are
        if (vertices.size() < 3) { return true; }

        // find the first non-zero triangle
        Triangle<element_t> triangle(vertices[0], vertices[1], vertices[2]);
        std::size_t idx = 3;
        while (idx < vertices.size() && triangle.is_zero()) {
            triangle.c = vertices[idx];
            ++idx;
        }

        // check if subsequent triangles always have the same orientation
        const Orientation opposite = -triangle.get_orientation();
        for (; idx <= vertices.size(); ++idx) {
            triangle.a = triangle.b;
            triangle.b = triangle.c;
            triangle.c = vertices[idx % vertices.size()];
            if (triangle.get_orientation() == opposite) { return false; }
        }
        return true;
    }
    constexpr bool is_convex() const noexcept { return is_convex(m_vertices); }
    /// @}

    /// Checks if this Polygon is concave.
    bool is_concave() const noexcept { return !is_convex(); }

    /// @{
    /// Calculates the orientation of a simple Polygon.
    /// From: http://geomalgorithms.com/a01-_area.html
    static Orientation get_orientation(const std::vector<vertex_t>& vertices) noexcept {
        if (vertices.size() < 3) { return Orientation::UNDEFINED; }

        std::size_t pivot = 0;
        { // find the righmost lowest vertex of the polygon
            V2f min = vertices[0];
            for (std::size_t idx = 1; idx < vertices.size(); ++idx) {
                if (vertices[idx].y() > min.y()) {
                    continue; // higher
                }
                if (vertices[idx].y() == min.y() && vertices[idx].x() < min.x()) {
                    continue; // same height but further left
                }
                pivot = idx;
                min = vertices[idx];
            }
        }
        const Triangle<element_t> triangle( //
            vertices[pivot == 0 ? vertices.size() - 1 : pivot - 1], vertices[pivot], vertices[pivot + 1]);
        return triangle.get_orientation();
    }
    Orientation get_orientation() const noexcept { return get_orientation(m_vertices); }
    /// @}

    /// @{
    /// Calculates the orientation of a complex Polygon.
    /// Is about 10x slower than the simple version.
    static Orientation get_orientation_general(const std::vector<vertex_t>& vertices) noexcept {
        // find three consecutive vertices that form a triangle that doesn't contain any other vertices
        Triangle<element_t> triangle;
        bool triangle_is_empty = false;
        for (size_t i = 2; i <= vertices.size() && !triangle_is_empty; ++i) {
            triangle = {vertices[i - 2], vertices[i - 1], vertices[i % vertices.size()]};
            triangle_is_empty = true;
            for (size_t j = 0; j < vertices.size(); ++j) {
                if (triangle.contains(vertices[j])) {
                    triangle_is_empty = false;
                    break;
                }
            }
        }
        if (triangle_is_empty) { return Orientation::UNDEFINED; }

        // the Polygon shares the orientation of the triangle if it is contained within
        const Orientation orientation = triangle.get_orientation();
        return contains(triangle.get_center()) ? orientation : -orientation;
    }
    Orientation get_orientation_general() const noexcept { return get_orientation_general(m_vertices); }
    /// @}

    /// @{
    /// Tests if the given point is fully contained in this Polygon.
    /// If the point is on the edge of the Polygon, it is not contained within it.
    /// @param point    Point to check.
    static bool contains(const std::vector<vertex_t>& vertices, const vertex_t& point) noexcept {
        return _get_winding_number(vertices, point) != 0;
    }
    constexpr bool contains(const vertex_t& point) const noexcept {
        return _get_winding_number(m_vertices, point) != 0;
    }
    /// @}

    /// Equality operator.
    constexpr bool operator==(const Polygon2& other) const noexcept { return m_vertices == other.m_vertices; }

    /// Inequality operator.
    constexpr bool operator!=(const Polygon2& other) const noexcept { return m_vertices != other.m_vertices; }

    /// Tests whether this Polygon is vertex-wise approximate to another.
    /// If you want to know if two Polygons are visually approximate, you'll have to optimize both first.
    /// @param other    Other Polygon to test against.
    /// @param epsilon  Largest ignored difference.
    bool is_approx(const Polygon2& other, const element_t epsilon = precision_high<element_t>()) const noexcept {
        if (other.m_vertices.size() != m_vertices.size()) { return false; }
        for (size_t i = 0; i < m_vertices.size(); ++i) {
            if (!m_vertices[i].is_approx(other.m_vertices[i], epsilon)) { return false; }
        }
        return true;
    }

    /// @{
    /// Remove all vertices that do not add additional corners to the Polygon.
    static void optimize(std::vector<vertex_t>& vertices) {
        // merge non-unique vertices (consecutive vertices that share the same position)
        remove_consecutive_equal(vertices);

        // do not store an explicit last vertex if the Polygon is closed anyway
        if (!vertices.empty() && vertices.front().is_approx(vertices.back())) { vertices.pop_back(); }

        { // remove vertices that they are on a straight line between their neighbours
            std::size_t rightmost_valid = 1;
            const std::size_t end = vertices.size();
            for (std::size_t front = 1; front < end; ++front) {
                const vertex_t& last = vertices[rightmost_valid - 1];
                vertex_t& current = vertices[rightmost_valid];
                const vertex_t& next = vertices[(front + 1) % end];
                if (!(current - last).is_parallel_to(next - current)) { ++rightmost_valid; }
                vertices[rightmost_valid] = next;
            }
            vertices.resize(rightmost_valid);
        }

        // may conserve some memory
        vertices.shrink_to_fit();
    }
    void optimize() { optimize(m_vertices); }
    /// @}

private:
    /// Calculate the winding number for a point with regard to this Polygon.
    /// The winding number is 0 iff the point lies outside the polygon.
    /// From: http://geomalgorithms.com/a03-_inclusion.html
    /// @param point    Point to test.
    static constexpr int _get_winding_number(const std::vector<vertex_t>& vertices, const vertex_t& point) noexcept {
        int winding_number = 0;
        for (std::size_t index = 0; index < vertices.size(); ++index) {
            const vertex_t& start = vertices[index];
            const vertex_t& end = vertices[(index + 1) % vertices.size()];
            if (start.y() <= point.y()) {
                if (point.y() < end.y() && shoelace(start, end, point) > 0) { // upward crossing
                    ++winding_number;
                }
            } else {                                                           // start.y() > point.y()
                if (point.y() >= end.y() && shoelace(start, end, point) < 0) { // downward crossing
                    --winding_number;
                }
            }
        }
        return winding_number;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Vertices of this Polygon.
    std::vector<vertex_t> m_vertices;
};

} // namespace detail

// conversions ====================================================================================================== //

/// Constructs a rectangular Polygon from an Aabr.
// template<>
// Polygon2f convert_to<Polygon2f, Aabrf>(const Aabrf& aabr);

// formatting ======================================================================================================= //

/// Prints the contents of a Polygon into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param polygon  Polygon to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const Polygon2f& polygon) {
    out << "Polygon2f(";
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
struct formatter<notf::detail::Polygon2<Element>> {
    using type = notf::detail::Polygon2<Element>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& polygon, FormatContext& ctx) {
        //        return format_to(ctx.begin(), "{}({}, {})", type::get_name(), aabr[0], aabr[1]);
        // TODO: Polygon fmt formatting (also print if Polygon is closed)
    }
};

} // namespace fmt

// std::hash ======================================================================================================== //

/// std::hash specialization for Triangle.
template<typename REAL>
struct std::hash<notf::detail::Polygon2<REAL>> {
    size_t operator()(const notf::detail::Polygon2<REAL>& polygon) const {
        auto result = notf::hash(static_cast<size_t>(notf::detail::HashID::POLYGON2));
        for (const auto& vertex : polygon.get_vertices()) {
            notf::hash_combine(result, vertex);
        }
        return result;
    }
};
