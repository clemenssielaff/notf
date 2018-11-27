#pragma once

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// keyboard ========================================================================================================= //

/// Modifier keys.
/// If you hold down more than one key of the same modifier (both shift-keys, for example),
/// the flag is still set only once (meaning there is no double-shift modifier).
enum KeyModifiers {
    NO_MODIFIER = 0,
    SHIFT = 1,
    CTRL = 2,
    ALT = 4,
    SUPER = 8,
};

/// All keys recognized by GLFW.
/// Can be used as indices for a KeyStateSet object.
enum class Key : unsigned char {
    __first = 0,
    SPACE = __first,
    APOSTROPHE,
    COMMA,
    MINUS,
    PERIOD,
    SLASH,
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    SEMICOLON,
    EQUAL,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LEFT_BRACKET,
    BACKSLASH,
    RIGHT_BRACKET,
    GRAVE_ACCENT,
    WORLD_1, // non-US #1
    WORLD_2, // non-US #2
    ESCAPE,
    ENTER,
    TAB,
    BACKSPACE,
    INSERT,
    DEL, // DELETE seems to be a reserved macro in MVSC
    RIGHT,
    LEFT,
    DOWN,
    UP,
    PAGE_UP,
    PAGE_DOWN,
    HOME,
    END,
    CAPS_LOCK,
    SCROLL_LOCK,
    NUM_LOCK,
    PRINT_SCREEN,
    PAUSE,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    KP_0,
    KP_1,
    KP_2,
    KP_3,
    KP_4,
    KP_5,
    KP_6,
    KP_7,
    KP_8,
    KP_9,
    KP_DECIMAL,
    KP_DIVIDE,
    KP_MULTIPLY,
    KP_SUBTRACT,
    KP_ADD,
    KP_ENTER,
    KP_EQUAL,
    LEFT_SHIFT,
    LEFT_CONTROL,
    LEFT_ALT,
    LEFT_SUPER,
    RIGHT_SHIFT,
    RIGHT_CONTROL,
    RIGHT_ALT,
    RIGHT_SUPER,
    MENU,
    INVALID = 255,
    __last = MENU,
};

/// Converts a GLFW key integer into a Key enum value.
Key to_key(int key);

/// Keyboard input.
struct KeyStroke {
    const Key key;               /// Key pressed (as determined by GLFW)
    const int scancode;          /// Sytem scancode, use to identify keys not recognized by GLFW.
    const KeyModifiers modifier; /// Modifiers pressed while the stroke was generated.
};

struct CharInput {
    const char32_t codepoint;   /// Character input
    const KeyModifiers modifiers;
};

// mouse ============================================================================================================ //

/// All mouse buttons recognized by GLFW.
/// Can be used as indices for a ButtonStateSet object.
enum class MouseButton {
    __first = 0,
    BUTTON_1 = __first,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7,
    BUTTON_8,
    NO_BUTTON,
    LEFT = BUTTON_1,
    RIGHT = BUTTON_2,
    MIDDLE = BUTTON_3,
    INVALID = 255,
    __last = BUTTON_8,
};

struct MouseClick {
    const MouseButton button;
    const KeyModifiers modifier;
};

NOTF_CLOSE_NAMESPACE
