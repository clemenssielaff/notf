/**
 * SIMD functions taken from:
 * https://github.com/g-truc/glm/blob/master/glm/simd/geometric.h
 */
#pragma once

#if 0

#include <emmintrin.h>

#include "common/xform4.hpp"

namespace notf {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

template <>
struct _Xform4<float, true, false> : public _Xform4<float, true, true> {

    using super = _Xform4<float, true, true>;

    _Xform4() = default;

    /** Perfect forwarding constructor. */
    template <typename... T>
    _Xform4(T&&... ts)
        : super{std::forward<T>(ts)...} {}

    // SIMD specializations *******************************************************************************************/
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace notf

#endif
