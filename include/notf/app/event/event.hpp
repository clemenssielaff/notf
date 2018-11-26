#pragma once

#include "notf/common/variant.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// forwards ========================================================================================================= //

struct KeyPressEvent;
struct KeyRepeatEvent;
struct KeyReleaseEvent;
struct CharInputEvent;
struct ShortcutEvent;
struct MouseButtonPressEvent;
struct MouseButtonReleaseEvent;
struct MouseMoveEvent;
struct MouseScrollEvent;
struct WindowCursorEnteredEvent;
struct WindowCursorExitedEvent;
struct WindowCloseEvent;
struct WindowMoveEvent;
struct WindowResizeEvent;
struct WindowResolutionChangeEvent;
struct WindowFocusGainEvent;
struct WindowFocusLostEvent;
struct WindowRefreshEvent;
struct WindowMinimizeEvent;
struct WindowRestoredEvent;
struct WindowFileDropEvent;

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
    KeyPressEvent,                           //
    KeyRepeatEvent,                          //
    KeyReleaseEvent,                         //
    CharInputEvent,                          //
    ShortcutEvent,                           //
    MouseButtonPressEvent,                   //
    MouseButtonReleaseEvent,                 //
    MouseMoveEvent,                          //
    MouseScrollEvent,                        //
    WindowCursorEnteredEvent,                //
    WindowCursorExitedEvent,                 //
    WindowCloseEvent,                        //
    WindowMoveEvent,                         //
    WindowResizeEvent,                       //
    WindowResolutionChangeEvent,             //
    WindowFocusGainEvent,                    //
    WindowFocusLostEvent,                    //
    WindowRefreshEvent,                      //
    WindowMinimizeEvent,                     //
    WindowRestoredEvent,                     //
    WindowFileDropEvent                      //
    >;

NOTF_CLOSE_NAMESPACE
