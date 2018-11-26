#pragma once

#include "notf/common/variant.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

struct WindowResizeEvent;

// any event ======================================================================================================== //

namespace detail {

template<class...>
struct wrap_event_types;
template<class... Ts>
struct wrap_event_types<std::tuple<Ts...>> {
    using type = std::variant<std::unique_ptr<Ts>...>;
};
template<class... Ts>
using wrap_event_types_t = typename wrap_event_types<std::tuple<Ts...>>::type;

} // namespace detail

using AnyEvent = detail::wrap_event_types_t< //
    WindowResizeEvent                        //
    >;

NOTF_CLOSE_NAMESPACE
