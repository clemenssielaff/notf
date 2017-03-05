#pragma once

#include "common/float.hpp"
#include "common/random.hpp"
#include "common/random.hpp"
#include "common/vector2.hpp"

namespace detail {

/** The largest screen diagonale that you can reasonably expect NoTF to encounter.
 * The value is based on the 8K fulldome format (8192 × 8192) used for planetaria...
 * It is higher than the theatric 8K resolution (10240 × 4320) and over twice as much as the normal 8K (7680 × 4320).
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

} // namespace notf
