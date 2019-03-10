#include "notf/common/geo/matrix3.hpp"

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/polybezier.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

/// Generic Vector2 transformation.
template<class Vector2Element, class MatrixElement>
constexpr detail::Vector2<Vector2Element> do_transform(const detail::Vector2<Vector2Element>& vector,
                                                       const detail::Matrix3<MatrixElement>& matrix) noexcept {
    return {
        matrix.data[0][0] * vector[0] + matrix.data[1][0] * vector[1] + matrix.data[2][0],
        matrix.data[0][1] * vector[0] + matrix.data[1][1] * vector[1] + matrix.data[2][1],
    };
}

/// Generic Aabr transformation.
template<class AabrElement, class MatrixElement>
constexpr detail::Aabr<AabrElement> do_transform(const detail::Aabr<AabrElement>& aabr,
                                                 const detail::Matrix3<MatrixElement>& matrix) noexcept {

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

/// Generic Polyline transformation.
template<class PolylineElement, class MatrixElement>
detail::Polyline<PolylineElement> do_transform(const detail::Polyline<PolylineElement>& Polyline,
                                               const detail::Matrix3<MatrixElement>& matrix) noexcept {
    using vertex_t = typename detail::Polyline<PolylineElement>::vector_t;

    std::vector<vertex_t> vertices;
    vertices.reserve(Polyline.get_vertex_count());

    for (const auto& vertex : Polyline.get_vertices()) {
        vertices.emplace_back(do_transform(vertex, matrix));
    }

    return detail::Polyline<PolylineElement>(std::move(vertices));
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

// Polyline * m3
template<> Polylinef transform_by<Polylinef, M3f>(const Polylinef& value, const M3f& matrix) { return do_transform(value, matrix); }

// bezier * m3
template<> CubicPolyBezier2f transform_by<CubicPolyBezier2f, M3f>(const CubicPolyBezier2f& value, const M3f& matrix) { return do_transform(value.get_hull(), matrix); }
template<> CubicPolyBezier2d transform_by<CubicPolyBezier2d, M3f>(const CubicPolyBezier2d& value, const M3f& matrix) { return do_transform(value.get_hull(), matrix); }

template<> CubicPolyBezier2f transform_by<CubicPolyBezier2f, M3d>(const CubicPolyBezier2f& value, const M3d& matrix) { return do_transform(value.get_hull(), matrix); }
template<> CubicPolyBezier2d transform_by<CubicPolyBezier2d, M3d>(const CubicPolyBezier2d& value, const M3d& matrix) { return do_transform(value.get_hull(), matrix); }

// clang-format on
NOTF_CLOSE_NAMESPACE
