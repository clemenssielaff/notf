#include "notf/common/geo/path2.hpp"

NOTF_USING_NAMESPACE;

// path2 ============================================================================================================ //

Path2::SubPath::SubPath(CubicPolyBezier2f path)
    : m_path(std::move(path))
    , center(m_path.get_hull().get_center())
    , segment_count(narrow_cast<uint>(m_path.get_segment_count()))
    , is_convex(m_path.get_hull().is_convex())
    , is_closed(m_path.get_hull().is_closed()) {}

Path2::Path2(std::vector<CubicPolyBezier2f> subpaths) {
    // remove empty subpaths
    m_subpaths.reserve(subpaths.size());
    for (auto& subpath : subpaths) {
        if (!subpath.m_hull.is_empty()) { m_subpaths.emplace_back(SubPath(std::move(subpath))); }
    }
    m_subpaths.shrink_to_fit();
}

Path2Ptr Path2::rect(const Aabrf& aabr) {
    auto line = Polylinef(aabr.get_bottom_left(), aabr.get_bottom_right(), //
                          aabr.get_top_right(), aabr.get_top_left());
    line.set_closed();
    std::vector<CubicPolyBezier2f> subpaths = {std::move(line)};
    return _create_shared(std::move(subpaths));
}
