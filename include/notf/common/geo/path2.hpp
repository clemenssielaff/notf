#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/geo/polybezier.hpp"

NOTF_OPEN_NAMESPACE

// TODO I wonder if Path2 should be moved to `common` or even `graphics/plotter`?

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
        friend Path2;

        // methods ------------------------------------------------------------
    public:
        /// Constructor.
        /// @param path Subpath.
        SubPath(CubicPolyBezier2f path);

        /// Position of a vector on the PolyBezier.
        /// The `t` argument is clamped to [0, n+1] for open hulls and [-(n+1), n+1] for closed ones, with n == number
        /// of bezier segments. If the hull is empty, the zero vector is returned.
        /// @param t    Position on the spline, integral part identifies the spline segment, fractional part the
        ///             position on that spline.
        V2f interpolate(const float t) const { return m_path.interpolate(t); }

        /// Returns the Parametric Bezier with the given index.
        /// @param index    Index, must be in the range [0, n] for open subpaths and [0, n+1] for closed subpaths, with
        ///                 n being the number of (complete) segments.
        CubicBezier2f get_segment(const size_t index) const { return m_path.get_segment(index); }

        // fields -------------------------------------------------------------
    private:
        /// Subpath,
        CubicPolyBezier2f m_path;

    public:
        /// Center position of all vertices of this Path.
        V2f center;

        /// Number of segments in this subpath.
        uint segment_count;

        /// Whether this Path is convex or concave.
        bool is_convex;

        /// Whether this Path is closed.
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
    static Path2Ptr create(CubicPolyBezier2f path) {
        return _create_shared(std::vector<CubicPolyBezier2f>{std::move(path)});
    }

    /// Rectangle.
    static Path2Ptr rect(const Aabrf& aabr);
    // TODO: other primitive Path2 shapes (rounded rect, ellipse etc.).

    /// Whether or not this Path2 contains any subpaths.
    bool is_empty() const { return m_subpaths.empty(); }

    /// Read access to all subpaths.
    const std::vector<SubPath>& get_subpaths() const { return m_subpaths; }

    /// The total number of vertices in all subpaths.
    size_t get_vertex_count() const {
        size_t result = 0;
        for (const auto& subpath : m_subpaths) {
            result += subpath.m_path.get_vertex_count();
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
    std::vector<SubPath> m_subpaths;
};

NOTF_CLOSE_NAMESPACE
