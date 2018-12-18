#include "notf/common/matrix3.hpp"

#include "notf/common/aabr.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

/// Generic Vector2 transformation.
template<class Vector2Element, class MatrixElement>
constexpr detail::Vector2<Vector2Element>
transform(const detail::Vector2<Vector2Element>& vector, const detail::Matrix3<MatrixElement>& matrix) noexcept {
    return {
        matrix.data[0][0] * vector[0] + matrix.data[1][0] * vector[1] + matrix.data[2][0],
        matrix.data[0][1] * vector[0] + matrix.data[1][1] * vector[1] + matrix.data[2][1],
    };
}

/// Generic Aabr transformation.
template<class AabrElement, class MatrixElement>
constexpr detail::Aabr<AabrElement>
transform(const detail::Aabr<AabrElement>& aabr, const detail::Matrix3<MatrixElement>& matrix) noexcept {
    detail::Vector2<AabrElement> d0(aabr.left(), aabr.bottom());
    detail::Vector2<AabrElement> d1(aabr.right(), aabr.top());
    detail::Vector2<AabrElement> d2(aabr.left(), aabr.top());
    detail::Vector2<AabrElement> d3(aabr.right(), aabr.bottom());
    d0 = transform(d0, matrix);
    d1 = transform(d1, matrix);
    d2 = transform(d2, matrix);
    d3 = transform(d3, matrix);
    return detail::Aabr<AabrElement>(min(d0.x(), d1.x(), d2.x(), d3.x()), // left
                                     min(d0.y(), d1.y(), d2.y(), d3.y()), // bottom
                                     max(d0.x(), d1.x(), d2.x(), d3.x()), // right
                                     max(d0.y(), d1.y(), d2.y(), d3.y())  // top
    );
}

} // namespace

NOTF_OPEN_NAMESPACE

// matrix3 ========================================================================================================== //

template<>
V2f transform_by<V2f, M3f>(const V2f& value, const M3f& matrix) {
    return transform(value, matrix);
}

template<>
Aabrf transform_by<Aabrf, M3f>(const Aabrf& value, const M3f& matrix) {
    return transform(value, matrix);
}

NOTF_CLOSE_NAMESPACE
