#include "notf/common/geo/path2.hpp"

NOTF_USING_NAMESPACE;

// path2 ============================================================================================================ //

Path2::SubPath::SubPath(CubicPolyBezier2f path)
    : path(std::move(path))
    , center(path.get_hull().get_center())
    , is_convex(path.get_hull().is_convex())
    , is_closed(path.get_hull().is_closed()) {}

Path2::Path2(std::vector<CubicPolyBezier2f> subpaths) {
    // remove empty subpaths
    m_subpaths.reserve(subpaths.size());
    for (auto& subpath : subpaths) {
        if (!subpath.m_hull.is_empty()) { m_subpaths.emplace_back(SubPath(std::move(subpath))); }
    }
    m_subpaths.shrink_to_fit();
}

Path2Ptr Path2::rect(const Aabrf& aabr) {
    std::vector<CubicPolyBezier2f> subpaths = {Polylinef(aabr.get_bottom_left(), aabr.get_bottom_right(), //
                                                         aabr.get_top_right(), aabr.get_top_left())};
    return _create_shared(std::move(subpaths));
}
