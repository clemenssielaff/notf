#pragma once

#include <algorithm>
#include <iosfwd>

#include "common/exception.hpp"
#include "common/segment.hpp"
#include "common/vector2.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

//====================================================================================================================//

/// Baseclass for simple Polygons.
/// "Simple" means that the Polygon has no (non-consecutive) vertices that share the same position, no intersecting
/// edges and must contain at least 3 unique points. Polygons are always closed, meaning the last point is always
/// implicitly connected to the first one.
template<typename REAL>
struct Polygon {

    /// Vector type.
    using vector_t = RealVector2<REAL>;

    /// Element type.
    using element_t = typename vector_t::element_t;

    /// Orientation type.
    using Orientation = typename Triangle<element_t>::Orientation;

    // fields --------------------------------------------------------------------------------------------------------//
    /// Vertices of this Polygon.
    const std::vector<vector_t> vertices;

    // methods -------------------------------------------------------------------------------------------------------//
    /// Value constructor.
    /// @param vertices         Vertices from which to construct the Polygon.
    /// @throws runtime_error   If the Polygon does not contain at least 3 unique vertices.
    /// @throws runtime_error   If two non-consecutive vertices share the same position.
    /// @throws runtime_error   If two edges of the Polygon intersect.
    Polygon(std::vector<vector_t> vertices) : vertices(_prepare_vertices(std::move(vertices))) {}

    /// Calculates the orientation of the Polygon.
    Orientation orientation() const
    {
        // find three consecutive vertices that form a triangle that doesn't contain any other vertices
        Triangle<element_t> triangle;
        bool is_empty;
        for (size_t i = 2; i <= vertices.size(); ++i) {
            triangle = {vertices[i - 2], vertices[i - 1], vertices[i % vertices.size()]};
            is_empty = true;
            for (size_t j = 0; j < vertices.size(); ++j) {
                if (triangle.contains(vertices[j])) {
                    is_empty = false;
                    break;
                }
            }
            if (is_empty) {
                break;
            }
        }
        assert(is_empty);

        // the Polygon shares the orientation of the triangle if it is contained
        if (this->contains(triangle.center())) {
            return triangle.orientation();
        }
        else {
            return (triangle.orientation() == Orientation::CCW) ? Orientation::CW : Orientation::CCW;
        }
    }

    /// The center point of the Polygon.
    vector_t center() const
    {
        vector_t result = vector_t::zero();
        for (const vector_t& vertex : vertices) {
            result += vertex;
        }
        result /= static_cast<element_t>(vertices.size());
        return result;
    }

    /// Tests if the point is fully contained in the Polygon.
    /// If the point is on the edge of the Polygon, it is not contained within it.
    /// @param point    Point to check.
    bool contains(const vector_t& point) const
    {
        // create a line segment from the point to some point on the outside of the polygon
        const Segment2<element_t> line(
            point, vector_t(1, 1)
                       + *(std::max_element(std::begin(vertices), std::end(vertices),
                                            [](const auto& lhs, const auto& rhs) { return lhs.x() < rhs.x(); })));

        // find the index of the first vertex that does not fall on the line
        size_t index = 0;
        while (index < vertices.size() && line.contains(vertices[index])) {
            ++index;
        }

        // count the number of intersections with segments of this Polygon
        uint intersections = 0;
        while (index < vertices.size()) {
            const vector_t& last_vertex    = vertices[index++];
            const vector_t& current_vertex = vertices[index % vertices.size()];
            if (Segment2<element_t>(last_vertex, current_vertex).intersects(line)) {
                ++intersections;
            }

            // if the current vertex falls directly onto the line, the line either crosses or touches it
            if (line.contains(current_vertex)) {
                do {
                    ++index; // skip all vertices that fall on the line
                } while (line.contains(vertices[index % vertices.size()]));
                const vector_t& current_vertex = vertices[index % vertices.size()];

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
    bool is_convex() const
    {
        // find the first non-zero triangle
        size_t index = 2;
        while (index < vertices.size() && Triangle<element_t>(vertices[0], vertices[1], vertices[index]).is_zero()) {
            ++index;
        }
        const Orientation first_orientation
            = Triangle<element_t>(vertices[0], vertices[1], vertices[index]).orientation();

        // check if subsequent triangles always have the same orientation
        for (size_t i = index + 1; i <= vertices.size(); ++i) {
            const Triangle<element_t> triangle(vertices[i - 2], vertices[i - 1], vertices[i % vertices.size()]);
            if (!triangle.is_zero() && triangle.orientation() != first_orientation) {
                return false;
            }
        }
        return true;
    }

    /// Checks if this Polygon is concave.
    bool is_concave() const { return !is_convex(); }

private:
    /// Enforces the construction of a simple Polygon with unique vertices.
    /// @param vertices Vertices from which to construct the Polygon.
    /// @throws runtime_error   If the Polygon does not contain at least 3 unique vertices.
    /// @throws runtime_error   If two non-consecutive vertices share the same position.
    /// @throws runtime_error   If two edges of the Polygon intersect.
    static std::vector<vector_t> _prepare_vertices(std::vector<vector_t>&& vertices)
    {
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
                    if (!vertices[i].is_approx(vertices[i - 1])) {
                        unique_vertices.emplace_back(vertices[i]);
                    }
                }

                std::swap(vertices, unique_vertices);
            }
        }
        vertices.shrink_to_fit();

        if (vertices.size() < 3) {
            notf_throw(runtime_error, "A Polygon must contain at least 3 unique vertices");
        }

        for (size_t i = 0; i < vertices.size(); ++i) {
            for (size_t j = i + 1; j < vertices.size(); ++j) {
                if (vertices[i].is_approx(vertices[j])) {
                    notf_throw(runtime_error, "Vertices in a Polygon must not share positions");
                }
            }
        }

        // TODO: this seems broken
//        for (size_t i = 1; i < vertices.size(); ++i) {
//            for (size_t j = i + 1; j < vertices.size(); ++j) {
//                if (Segment2<element_t>(vertices[i - 1], vertices[i])
//                        .intersects(Segment2<element_t>(vertices[j - 1], vertices[j]))) {
//                    notf_throw(runtime_error, "Segments in a Polygon may not intersect");
//                }
//            }
//        }

        return vertices;
    }
};

} // namespace detail

//====================================================================================================================//

using Polygonf = detail::Polygon<float>;

//====================================================================================================================//

/// Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param polygon  Polygon to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Polygonf& polygon);

NOTF_CLOSE_NAMESPACE

//====================================================================================================================//

namespace std {

/// std::hash specialization for Triangle.
template<typename REAL>
struct hash<notf::detail::Polygon<REAL>> {
    size_t operator()(const notf::detail::Polygon<REAL>& polygon) const
    {
        auto result = notf::hash(static_cast<size_t>(notf::detail::HashID::POLYGON));
        for (const auto& vertex : polygon.vertices) {
            notf::hash_combine(result, vertex);
        }
        return result;
    }
};

} // namespace std
