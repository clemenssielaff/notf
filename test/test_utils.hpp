#pragma once

#include "common/float.hpp"
#include "common/random.hpp"
#include "common/random.hpp"
#include "common/vector2.hpp"
#include "common/xform2.hpp"

namespace detail {

/** The largest screen diagonale that you can reasonably expect NoTF to encounter.
 * The value is based on the 8K fulldome format (8192 × 8192) used for planetaria...
 * It is higher than the theatric 8K resolution (10240 × 4320) and over twice as much as normal 8K (7680 × 4320).
 */
constexpr long double largest_supported_diagonale() { return 11585.2375029603946397834340287258466596427520030879586l; }

} // namespace detail

namespace notf {

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

template <typename Real>
inline _RealVector2<Real> random_vector()
{
    return _RealVector2<Real>(random_number<Real>(lowest_tested<Real>(), highest_tested<Real>()),
                              random_number<Real>(lowest_tested<Real>(), highest_tested<Real>()));
}

template <typename Real>
inline _RealVector2<Real> random_vector(const Real minimum, const Real maximum)
{
    return _RealVector2<Real>(random_number<Real>(minimum, maximum), random_number<Real>(minimum, maximum));
}

template <typename Real>
inline _Xform2<Real> random_xform2(const Real min_trans, const Real max_trans, const Real min_scale, const Real max_scale)
{
    _Xform2<Real> result = _Xform2<Real>::scale(random_number(min_scale, max_scale));
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
