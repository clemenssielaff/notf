#include "notf/common/geo/polyline.hpp"

NOTF_OPEN_NAMESPACE

// polyline ========================================================================================================= //

template<>
Polylinef convert_to<Polylinef, Aabrf>(const Aabrf& aabr) {
    std::vector<Polylinef::vector_t> points;
    points.reserve(4);
    points.emplace_back(aabr.get_bottom_left());
    points.emplace_back(aabr.get_bottom_right());
    points.emplace_back(aabr.get_top_right());
    points.emplace_back(aabr.get_top_left());
    return Polylinef(std::move(points));
}

NOTF_CLOSE_NAMESPACE
