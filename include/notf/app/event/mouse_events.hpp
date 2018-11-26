#pragma once

#include "notf/app/event/input.hpp"
#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// mouse events ===================================================================================================== //

/// Generated when a mouse button was pressed while a notf Window had the OS focus.
struct MouseButtonPressEvent {
    const WindowHandle window;
    const MouseButton button;
    const KeyModifiers modifier;
};

/// Generated when a mouse button was released while a notf Window had the OS focus.
struct MouseButtonReleaseEvent {
    const WindowHandle window;
    const MouseButton button;
    const KeyModifiers modifier;
};

/// Generated when the mouse cursor is moved while a notf Window had the OS focus.
struct MouseMoveEvent {
    const WindowHandle window;
    const V2d position; // position of the cursor in window coordinates
};

/// Generated when the mouse is scrolled while a notf Window had the OS focus.
struct MouseScrollEvent {
    const WindowHandle window;
    const V2d delta; // scroll delta in screen coordinates
};

NOTF_CLOSE_NAMESPACE
