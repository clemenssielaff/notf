#pragma once

#include "notf/app/event/input.hpp"
#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// keyboard events ================================================================================================== //

/// Generated when a keyboard key was pressed while a notf Window had the OS focus.
struct KeyPressEvent {
    const WindowHandle window;
    const Key key;
    const int scancode;
    const KeyModifiers modifier;
};

/// Generated when a keyboard key was held down while a notf Window had the OS focus.
struct KeyRepeatEvent {
    const WindowHandle window;
    const Key key;
    const int scancode;
    const KeyModifiers modifier;
};

/// Generated when a keyboard key was released while a notf Window had the OS focus.
struct KeyReleaseEvent {
    const WindowHandle window;
    const Key key;
    const int scancode;
    const KeyModifiers modifier;
};

/// Generated when an unicode code point was generated.
struct CharInputEvent {
    const WindowHandle window;
    const uint codepoint;
};

/// Generated when the user presses a key combination with at least one modifier key.
struct ShortcutEvent {
    const WindowHandle window;
    const uint codepoint;
    const KeyModifiers modifiers;
};

NOTF_CLOSE_NAMESPACE
