#include "notf/common/geo/path2.hpp"

NOTF_USING_NAMESPACE;

// TODO: normalize Path2::Paths

// path2::path ====================================================================================================== //

void Path2::Path::add_subpath(std::initializer_list<Vertex>&& vertices) {
    // extract subpath information
    Subpath subpath;
    subpath.first_index = m_vertices.size();
    subpath.size = vertices.size();
    subpath.is_convex = true; // TODO is convex?

    // store the vertices
    extend(m_vertices, vertices);
    subpath.is_closed
        = m_vertices[subpath.first_index].pos.is_approx(m_vertices[subpath.first_index + subpath.size - 1].pos);

    // calculate tangents for straight edges
    for (size_t i = subpath.first_index + 1; i < subpath.first_index + subpath.size; ++i) {
        Vertex& last_vertex = m_vertices[i - 1];
        Vertex& current_vertex = m_vertices[i];
        if (last_vertex.right_tangent.is_zero()) {
            last_vertex.right_tangent = (current_vertex.pos - last_vertex.pos).normalize();
        }
        if (current_vertex.left_tangent.is_zero()) {
            current_vertex.left_tangent = (last_vertex.pos - current_vertex.pos).normalize();
        }
    }

    // if the subpath is closed, we don't need to store the last vertex
    // just get its tangent and pop it
    if (subpath.is_closed) {
        Vertex first_vertex = m_vertices[subpath.first_index];
        Vertex last_vertex = m_vertices[subpath.first_index + subpath.size - 1];
        first_vertex.left_tangent = last_vertex.left_tangent;
        subpath.size -= 1;
        m_vertices.pop_back();
    }

    // store the subpath
    m_subpaths.emplace_back(std::move(subpath));
}

size_t Path2::Path::get_hash() {
    size_t result = to_number(notf::detail::HashID::PATH2);
    hash_combine(result, m_vertices);
    return result;
}

// path2 ============================================================================================================ //

Path2 Path2::rect() {
    PathPtr path = std::make_shared<Path>();
    path->add_subpath(V2d{-1, -1}, V2d{1, -1}, V2d{1, 1}, V2d{-1, 1}, V2d{-1, -1});
    return path;
}

// std::hash ======================================================================================================== //

/// std::hash specialization for notf::Path2::Path::Vertex.
template<>
struct std::hash<::notf::Path2::Path::Vertex> {
    size_t operator()(::notf::Path2::Path::Vertex const& vertex) const noexcept {
        return ::notf::hash(vertex.pos, vertex.left_tangent, vertex.right_tangent, vertex.left_tangent,
                            vertex.right_tangent);
    }
};
