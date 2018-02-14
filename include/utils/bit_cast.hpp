#pragma once

#include <string.h> // for memcpy
#include <type_traits>

namespace notf {

/// Save bit_cast.
/// adpted from https://chromium.googlesource.com/chromium/src/+/1587f7d/base/macros.h#76
template<typename targetT, typename sourceT>
inline targetT bit_cast(const sourceT& source)
{
    static_assert(sizeof(sourceT) == sizeof(targetT), "bit_cast requires both types to be of the same size");
    static_assert(std::is_pod<sourceT>::value, "bit_cast source and target types must be PODs");
    static_assert(std::is_pod<targetT>::value, "bit_cast source and target types must be PODs");
    targetT target;
    memcpy(&target, &source, sizeof(target));
    return target;
}

} // namespace notf
