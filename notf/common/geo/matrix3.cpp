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

    detail::Vector2<AabrElement> d0(aabr.get_left(), aabr.get_bottom());
    detail::Vector2<AabrElement> d1(aabr.get_right(), aabr.get_top());
    detail::Vector2<AabrElement> d2(aabr.get_left(), aabr.get_top());
    detail::Vector2<AabrElement> d3(aabr.get_right(), aabr.get_bottom());

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
template<class V, class M, class Value = detail::Polyline<V>, class Matrix = detail::Matrix3<M>>
Value do_transform(const Value& polyline, const Matrix& matrix) noexcept {
    using vertex_t = typename Value::vector_t;

    std::vector<vertex_t> vertices;
    vertices.reserve(polyline.get_vertex_count());

    for (const auto& vertex : polyline.get_vertices()) {
        vertices.emplace_back(do_transform(vertex, matrix));
    }

    Value result(std::move(vertices), polyline.is_closed());
    return result;
}

/// Generic PolyBezier transformation.
template<size_t D, class V, class M, class Value = detail::PolyBezier<D, V>, class Matrix = detail::Matrix3<M>>
Value do_transform(const Value& polybezier, const Matrix& matrix) noexcept {
    Value result;
    result.m_hull = do_transform<V, M>(polybezier.get_hull(), matrix);
    return result;
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
template<> Polylinef transform_by<Polylinef, M3f>(const Polylinef& value, const M3f& matrix) { return do_transform<float, float>(value, matrix); }

// bezier * m3
template<> CubicPolyBezier2f transform_by<CubicPolyBezier2f, M3f>(const CubicPolyBezier2f& value, const M3f& matrix) { return do_transform<3, float, float>(value, matrix); }
template<> CubicPolyBezier2d transform_by<CubicPolyBezier2d, M3f>(const CubicPolyBezier2d& value, const M3f& matrix) { return do_transform<3, double, float>(value, matrix); }

template<> CubicPolyBezier2f transform_by<CubicPolyBezier2f, M3d>(const CubicPolyBezier2f& value, const M3d& matrix) { return do_transform<3, float, double>(value, matrix); }
template<> CubicPolyBezier2d transform_by<CubicPolyBezier2d, M3d>(const CubicPolyBezier2d& value, const M3d& matrix) { return do_transform<3, double, double>(value, matrix); }

// clang-format on
NOTF_CLOSE_NAMESPACE
