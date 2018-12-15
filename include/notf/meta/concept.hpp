#pragma once

#include "notf/meta/types.hpp"

// macros =========================================================================================================== //

#define NOTF_CREATE_TYPE_DETECTOR(name)                                                            \
    template<class Investigated, class = void>                                                     \
    struct has_##name : std::false_type {};                                                        \
    template<class Investigated>                                                                   \
    struct has_##name<Investigated, std::void_t<typename Investigated::name>> : std::true_type {}; \
    template<class Investigated>                                                                   \
    static constexpr bool has_##name##_v = decltype(has_##name<Investigated>())::value

#define NOTF_CREATE_FIELD_DETECTOR(name)                                                            \
    template<class Investigated, class = void>                                                      \
    struct has_##name : std::false_type {};                                                         \
    template<class Investigated>                                                                    \
    struct has_##name<Investigated, std::void_t<decltype(Investigated::name)>> : std::true_type {}; \
    template<class Investigated>                                                                    \
    static constexpr bool has_##name##_v = decltype(has_##name<Investigated>())::value
