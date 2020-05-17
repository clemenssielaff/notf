#include "notf/common/geo/polygon2.hpp"

NOTF_OPEN_NAMESPACE

// polygon2 ========================================================================================================= //

template<>
Polygon2f convert_to<Polygon2f, Aabrf>(const Aabrf& aabr) {
    std::vector<Polygon2f::vertex_t> vertices;
    vertices.reserve(4);
    vertices.emplace_back(aabr.get_bottom_left());
    vertices.emplace_back(aabr.get_bottom_right());
    vertices.emplace_back(aabr.get_top_right());
    vertices.emplace_back(aabr.get_top_left());
    return Polygon2f(std::move(vertices));
}

NOTF_CLOSE_NAMESPACE
