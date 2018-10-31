#pragma once

//#include "common/arithmetic.hpp"
//#include "common/float.hpp"
//#include "common/matrix3.hpp"
//#include "common/matrix4.hpp"
//#include "common/padding.hpp"
//#include "common/random.hpp"
//#include "common/size2.hpp"
//#include "common/vector2.hpp"

#include <thread>

#include "notf/common/random.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/types.hpp"

NOTF_OPEN_NAMESPACE

namespace detail {

/// The largest screen diagonale that you can reasonably expect NoTF to encounter.
/// The value is based on the 8K fulldome format (8192 × 8192) used for planetaria...
/// It is higher than the theatric 8K resolution (10240 × 4320) and over twice as much as normal 8K (7680 × 4320).
constexpr long double largest_supported_diagonale() { return 11585.2375029603946397834340287258466596427520030879586l; }

} // namespace detail

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

/// Random number around zero in the range of a size what we'd expect to see as a monitor resolution.
template<typename Real>
inline Real random_tested()
{
    return random<Real>(lowest_tested<Real>(), highest_tested<Real>());
}

/// Generates a std::thread::id, even though its constructor is private.
inline std::thread::id make_thread_id(const uint number)
{
    using id_as_number = templated_unsigned_integer_t<bitsizeof<std::thread::id>()>;
    static_assert(max_value<decltype(number)>() <= max_value<id_as_number>(),
                  "Cannot reliably represent an unsigned integer as a std::thread::id, because it is too large to fit");
    std::thread::id id;
    auto id_ptr = std::launder(reinterpret_cast<id_as_number*>(&id));
    *id_ptr = number;
    return id;
}

NOTF_CLOSE_NAMESPACE
