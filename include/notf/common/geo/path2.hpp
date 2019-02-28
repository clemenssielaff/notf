#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/geo/polybezier.hpp"

NOTF_OPEN_NAMESPACE

// path2 ============================================================================================================ //

/// A Path is a collection of immutable 2D Polybeziers that can be used as a resource in an application.
/// It has to be immutable because once created, it must be able to be passed around in a multithreaded enviroment
/// without the need for locks and checks.
/// Paths can be created from other geometrical constructs like circles, rects or polygons, or via the "Path2::Drawer"
/// that behaves like an HTML5 canvas painter.
/// A Path2 consists of 0-n "subpaths", each one a two-dimensional cubic
/// Polybezier that is either closed or open, in a clock- or counterclockwise orientation.
class Path2 {

    // TODO: HTML5-like Path2::Drawer factory class

    // types ----------------------------------------------------------------------------------- //
private:
    /// Struct storing additional information about each subpath for easy access.
    struct SubPath {

        // methods ------------------------------------------------------------

        /// Constructor.
        /// @param path Subpath.
        SubPath(CubicPolyBezier2f path);

        // fields -------------------------------------------------------------

        /// Subpath,
        CubicPolyBezier2f path;

        /// Center position of all vertices of this Path.
        V2f center;

        /// Whether this Path is convex or concave.
        bool is_convex;

        /// Whether this Path is closed or not.
        bool is_closed;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(Path2);

    /// Value constructor.
    Path2(std::vector<CubicPolyBezier2f> subpaths);

public:
    /// Single Path constructor.
    /// @parm path  Single subpath.
    Path2(CubicPolyBezier2f path) : Path2(std::vector<CubicPolyBezier2f>{std::move(path)}) {}

    /// Rectangle.
    static Path2Ptr rect(const Aabrf& aabr);
    // TODO: other primitive Path2 shapes (rounded rect, ellipse etc.).

    /// Whether or not this Path2 contains any subpaths.
    bool is_empty() const { return m_subpaths.empty(); }

    /// Read access to all subpaths.
    const std::vector<const SubPath>& get_subpaths() const { return m_subpaths; }

    /// The total number of vertices in all subpaths.
    size_t get_vertex_count() const {
        size_t result = 0;
        for (const auto& subpath : m_subpaths) {
            result += subpath.path.get_hull().get_size();
        }
        return result;
    }
    /// The center of all vertices in all subpaths.
    V2f get_center() const {
        if (is_empty()) { return V2f::zero(); }
        V2f center = m_subpaths.front().center;
        for (size_t i = 1; i < m_subpaths.size(); ++i) {
            center += m_subpaths[i].center;
        }
        center *= 1 / static_cast<float>(m_subpaths.size());
        return center;
    }

    /// A Path is considered convex if all of its subpaths are convex.
    bool is_convex() const {
        bool result = true;
        for (size_t i = 1; result && i < m_subpaths.size(); ++i) {
            result &= m_subpaths[i].is_convex;
        }
        return result;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All subpaths making up this Path2.
    std::vector<const SubPath> m_subpaths;
};

NOTF_CLOSE_NAMESPACE
