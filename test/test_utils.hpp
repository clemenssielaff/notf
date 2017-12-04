#pragma once

#include "common/arithmetic.hpp"
#include "common/float.hpp"
#include "common/matrix3.hpp"
#include "common/matrix4.hpp"
#include "common/padding.hpp"
#include "common/random.hpp"
#include "common/size2.hpp"
#include "common/vector2.hpp"

namespace notf {

namespace detail {

/** The largest screen diagonale that you can reasonably expect NoTF to encounter.
 * The value is based on the 8K fulldome format (8192 × 8192) used for planetaria...
 * It is higher than the theatric 8K resolution (10240 × 4320) and over twice as much as normal 8K (7680 × 4320).
 */
constexpr long double largest_supported_diagonale() { return 11585.2375029603946397834340287258466596427520030879586l; }

} // namespace detail

using detail::RealVector2;
using detail::Matrix3;
using detail::Size2;

template<typename Real>
constexpr Real lowest_tested()
{
    return static_cast<Real>(-detail::largest_supported_diagonale());
}

template<typename Real>
constexpr Real highest_tested()
{
    return static_cast<Real>(detail::largest_supported_diagonale());
}

/** Random number around zero in the range of a size what we'd expect to see as a monitor resolution. */
template<typename Real>
inline Real random_number()
{
    return random_number<Real>(lowest_tested<Real>(), highest_tested<Real>());
}

template<typename Real>
inline RealVector2<Real> lowest_vector()
{
    return RealVector2<Real>(lowest_tested<Real>(), lowest_tested<Real>());
}

template<typename Real>
inline RealVector2<Real> highest_vector()
{
    return RealVector2<Real>(highest_tested<Real>(), highest_tested<Real>());
}

template<typename T>
inline T random_vector(const typename T::element_t minimum = lowest_tested<typename T::element_t>(),
                       const typename T::element_t maximum = highest_tested<typename T::element_t>())
{
    T result;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = random_number<typename T::element_t>(minimum, maximum);
    }
    return result;
}

template<typename T>
inline T random_matrix(const typename T::element_t minimum = lowest_tested<typename T::element_t>(),
                       const typename T::element_t maximum = highest_tested<typename T::element_t>())
{
    T result;
    for (size_t i = 0; i < result.size(); ++i) {
        for (size_t j = 0; j < result[0].size(); ++j) {
            result[i][j] = random_number<typename T::element_t>(minimum, maximum);
        }
    }
    return result;
}

template<typename Real>
inline Matrix3<Real>
random_matrix3(const Real min_trans, const Real max_trans, const Real min_scale, const Real max_scale)
{
    Matrix3<Real> result = Matrix3<Real>::scaling(random_number(min_scale, max_scale));
    result *= Matrix3<Real>::rotation(random_radian<Real>());
    result *= Matrix3<Real>::translation(
        RealVector2<Real>(random_number(min_trans, max_trans), random_number(min_trans, max_trans)));
    return result;
}

template<typename Real>
inline Matrix3<Real> random_matrix3()
{
    return random_matrix3(lowest_tested<Real>(), highest_tested<Real>(), Real(0), Real(2));
}

template<typename T>
inline Size2<T> random_size(const T from, const T to)
{
    return {random_number<T>(from, to), random_number<T>(from, to)};
}

inline bool random_event(const double probability) { return random_number<double>(0, 1) < clamp(probability, 0, 1); }

inline Padding random_padding(const float from, const float to)
{
    return {random_number<float>(from, to), random_number<float>(from, to), random_number<float>(from, to),
            random_number<float>(from, to)};
}

} // namespace notf
