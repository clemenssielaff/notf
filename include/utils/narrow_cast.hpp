#pragma once

#include <stdexcept>

#include "common/exception.hpp"
#include "common/meta.hpp"

namespace notf {

/// Save narrowing cast.
/// https://github.com/Microsoft/GSL/blob/master/include/gsl/gsl_util
template<class TARGET, class RAW_SOURCE>
inline constexpr TARGET narrow_cast(RAW_SOURCE&& value)
{
    using SOURCE = std::decay_t<RAW_SOURCE>;

    TARGET result = static_cast<TARGET>(std::forward<RAW_SOURCE>(value));
    if (static_cast<SOURCE>(result) != value) {
        throw_notf_error_msg(logic_error, "narrow_cast failed");
    }

#ifdef NOTF_CPP17
    if constexpr (!is_same_signedness<TARGET, SOURCE>::value) {
#else
    {
#endif
        if ((result < TARGET{}) != (value < SOURCE{})) {
            throw_notf_error_msg(logic_error, "narrow_cast failed");
        }
    }

    return result;
}

} // namespace notf
