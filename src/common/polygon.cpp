#include "notf/common/polygon.hpp"

NOTF_OPEN_NAMESPACE

// polygon ========================================================================================================== //

template<>
Polygonf convert_to<Polygonf, Aabrf>(const Aabrf& aabr) {
    std::vector<Polygonf::vector_t> points;
    points.reserve(4);
    points.emplace_back(aabr.get_bottom_left());
    points.emplace_back(aabr.get_bottom_right());
    points.emplace_back(aabr.get_top_right());
    points.emplace_back(aabr.get_top_left());
    return Polygonf(std::move(points));
}

NOTF_CLOSE_NAMESPACE
