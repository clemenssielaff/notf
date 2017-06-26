#pragma once

#include "common/arithmetic.hpp"
#include "common/float.hpp"
#include "common/random.hpp"
#include "common/random.hpp"
#include "common/vector2.hpp"
#include "common/xform2.hpp"
#include "common/xform4.hpp"

namespace notf {

namespace detail {

/** The largest screen diagonale that you can reasonably expect NoTF to encounter.
 * The value is based on the 8K fulldome format (8192 × 8192) used for planetaria...
 * It is higher than the theatric 8K resolution (10240 × 4320) and over twice as much as normal 8K (7680 × 4320).
 */
constexpr long double largest_supported_diagonale() { return 11585.2375029603946397834340287258466596427520030879586l; }

} // namespace detail

template <typename Real>
constexpr Real lowest_tested() { return static_cast<Real>(-detail::largest_supported_diagonale()); }

template <typename Real>
constexpr Real highest_tested() { return static_cast<Real>(detail::largest_supported_diagonale()); }

/** Random number around zero in the range of a size what we'd expect to see as a monitor resolution. */
template <typename Real>
inline Real random_number()
{
    return random_number<Real>(lowest_tested<Real>(), highest_tested<Real>());
}

template <typename Real>
inline _RealVector2<Real> lowest_vector() { return _RealVector2<Real>(lowest_tested<Real>(), lowest_tested<Real>()); }

template <typename Real>
inline _RealVector2<Real> highest_vector() { return _RealVector2<Real>(highest_tested<Real>(), highest_tested<Real>()); }

template <typename T>
inline T random_vector(const typename T::value_t minimum = lowest_tested<typename T::value_t>(),
                       const typename T::value_t maximum = highest_tested<typename T::value_t>())
{
    T result;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = random_number<typename T::value_t>(minimum, maximum);
    }
    return result;
}

template <typename T>
inline T random_matrix(const typename T::value_t minimum = lowest_tested<typename T::value_t>(),
                       const typename T::value_t maximum = highest_tested<typename T::value_t>())
{
    T result;
    for (size_t i = 0; i < result.size(); ++i) {
        for (size_t j = 0; j < result[0].size(); ++j) {
            result[i][j] = random_number<typename T::value_t>(minimum, maximum);
        }
    }
    return result;
}

template <typename Real>
inline _Xform2<Real> random_xform2(const Real min_trans, const Real max_trans, const Real min_scale, const Real max_scale)
{
    _Xform2<Real> result = _Xform2<Real>::scaling(random_number(min_scale, max_scale));
    result *= _Xform2<Real>::rotation(random_radian<Real>());
    result *= _Xform2<Real>::translation(_RealVector2<Real>(random_number(min_trans, max_trans),
                                                            random_number(min_trans, max_trans)));
    return result;
}

template <typename Real>
inline _Xform2<Real> random_xform2()
{
    return random_xform2(lowest_tested<Real>(), highest_tested<Real>(), Real(0), Real(2));
}
} // namespace notf
