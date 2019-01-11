#include "notf/common/matrix3.hpp"

#include "notf/common/aabr.hpp"
#include "notf/common/bezier.hpp"
#include "notf/common/polygon.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

/// Generic Vector2 transformation.
template<class Vector2Element, class MatrixElement>
constexpr detail::Vector2<Vector2Element>
do_transform(const detail::Vector2<Vector2Element>& vector, const detail::Matrix3<MatrixElement>& matrix) noexcept {
    return {
        matrix.data[0][0] * vector[0] + matrix.data[1][0] * vector[1] + matrix.data[2][0],
        matrix.data[0][1] * vector[0] + matrix.data[1][1] * vector[1] + matrix.data[2][1],
    };
}

/// Generic Aabr transformation.
template<class AabrElement, class MatrixElement>
constexpr detail::Aabr<AabrElement>
do_transform(const detail::Aabr<AabrElement>& aabr, const detail::Matrix3<MatrixElement>& matrix) noexcept {

    detail::Vector2<AabrElement> d0(aabr.left(), aabr.bottom());
    detail::Vector2<AabrElement> d1(aabr.right(), aabr.top());
    detail::Vector2<AabrElement> d2(aabr.left(), aabr.top());
    detail::Vector2<AabrElement> d3(aabr.right(), aabr.bottom());

    d0 = do_transform(d0, matrix);
    d1 = do_transform(d1, matrix);
    d2 = do_transform(d2, matrix);
    d3 = do_transform(d3, matrix);

    return detail::Aabr<AabrElement>(min(d0.x(), d1.x(), d2.x(), d3.x()), // left
                                     min(d0.y(), d1.y(), d2.y(), d3.y()), // bottom
                                     max(d0.x(), d1.x(), d2.x(), d3.x()), // right
                                     max(d0.y(), d1.y(), d2.y(), d3.y())  // top
    );
}

/// Generic Polygon transformation.
template<class PolygonElement, class MatrixElement>
constexpr detail::Polygon<PolygonElement>
do_transform(const detail::Polygon<PolygonElement>& polygon, const detail::Matrix3<MatrixElement>& matrix) noexcept {
    using vertex_t = typename detail::Polygon<PolygonElement>::vector_t;

    std::vector<vertex_t> vertices;
    vertices.reserve(polygon.get_vertex_count());

    for (const auto& vertex : polygon.get_vertices()) {
        vertices.emplace_back(do_transform(vertex, matrix));
    }

    return detail::Polygon<PolygonElement>(std::move(vertices));
}

/// Generic Bezier transformation.
template<size_t Order, class BezierElement, class MatrixElement>
constexpr detail::Bezier<Order, BezierElement> do_transform(const detail::Bezier<Order, BezierElement>& spline,
                                                            const detail::Matrix3<MatrixElement>& matrix) noexcept {
    using point_t = typename detail::Bezier<Order, BezierElement>::vector_t;
    using segment_t = typename detail::Bezier<Order, BezierElement>::Segment;

    std::vector<segment_t> segments;
    segments.reserve(spline.segments.size());

    for (const segment_t& segment : spline.segments) {
        segments.emplace_back(point_t(do_transform(segment.start, matrix)),
                              point_t(do_transform(segment.ctrl1, matrix)),
                              point_t(do_transform(segment.ctrl2, matrix)),
                              point_t(do_transform(segment.end, matrix)));
    }
    return detail::Bezier<Order, BezierElement>(std::move(segments));
}

} // namespace

NOTF_OPEN_NAMESPACE

// transformations ================================================================================================== //
// clang-format off

// v2 * m3
template<> V2f transform_by<V2f, M3f>(const V2f& value, const M3f& matrix) { return do_transform(value, matrix); }
template<> V2d transform_by<V2d, M3f>(const V2d& value, const M3f& matrix) { return do_transform(value, matrix); }

template<> V2f transform_by<V2f, M3d>(const V2f& value, const M3d& matrix) { return do_transform(value, matrix); }
template<> V2d transform_by<V2d, M3d>(const V2d& value, const M3d& matrix) { return do_transform(value, matrix); }

// aabr * m3
template<> Aabrf transform_by<Aabrf, M3f>(const Aabrf& value, const M3f& matrix) { return do_transform(value, matrix); }
template<> Aabrd transform_by<Aabrd, M3f>(const Aabrd& value, const M3f& matrix) { return do_transform(value, matrix); }

template<> Aabrf transform_by<Aabrf, M3d>(const Aabrf& value, const M3d& matrix) { return do_transform(value, matrix); }
template<> Aabrd transform_by<Aabrd, M3d>(const Aabrd& value, const M3d& matrix) { return do_transform(value, matrix); }

// polygon * m3
template<> Polygonf transform_by<Polygonf, M3f>(const Polygonf& value, const M3f& matrix) { return do_transform(value, matrix); }

// bezier * m3
template<> CubicBezier2f transform_by<CubicBezier2f, M3f>(const CubicBezier2f& value, const M3f& matrix) { return do_transform(value, matrix); }
template<> CubicBezier2d transform_by<CubicBezier2d, M3f>(const CubicBezier2d& value, const M3f& matrix) { return do_transform(value, matrix); }

template<> CubicBezier2f transform_by<CubicBezier2f, M3d>(const CubicBezier2f& value, const M3d& matrix) { return do_transform(value, matrix); }
template<> CubicBezier2d transform_by<CubicBezier2d, M3d>(const CubicBezier2d& value, const M3d& matrix) { return do_transform(value, matrix); }

// clang-format on
NOTF_CLOSE_NAMESPACE
