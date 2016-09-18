#pragma once

#include <assert.h>
#include <bitset>

#include "utils/enum_to_number.hpp"

namespace signal {

class Window;

/// \brief All keys recognized by GLFW.
///
/// Can be used as indices for a KeyStateSet object.
enum class KEY {
    INVALID = -1,
    SPACE,
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
    DELETE,
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
    __last,
};

/// \brief Actions you can do with a key.
///
enum class KEY_ACTION {
    KEY_RELEASE = 0,
    KEY_PRESS,
    KEY_REPEAT,
};

/// \brief Modifier keys.
///
/// If you hold down more than one key of the same modifier (both shift-keys, for example),
/// the flag is still set only once (meaning there is no double-shift modifier).
enum KEY_MODIFIERS {
    MOD_SHIFT = 0x0001,
    MOD_CTRL = 0x0002,
    MOD_ALT = 0x0004,
    MOD_SUPER = 0x0008,
};

/// \brief  The state of all regonized keys in a compact bitset.
///
/// True means pressed, false unpressed.
/// Use KEY values as index to access individual key states.
using KeyStateSet = std::bitset<to_number(KEY::__last)>;

/// \brief Checks the state of a given key in the KeyStateSet.
///
/// \param state_set    KeyStateSet to test.
/// \param key          Key to test.
///
/// \return True iff the key is pressed, false otherwise.
inline bool test_key(const KeyStateSet& state_set, KEY key)
{
    assert(key > KEY::INVALID && key < KEY::__last);
    return state_set.test(static_cast<size_t>(to_number(key)));
}

/// \brief Sets the state of a given key in the KeyStateSet.
///
/// \param state_set    KeyStateSet to change.
/// \param key          Key to modify.
/// \param state        Whether the key is pressed or not.
///
inline void set_key(KeyStateSet& state_set, KEY key, bool state)
{
    assert(key > KEY::INVALID && key < KEY::__last);
    state_set.set(static_cast<size_t>(to_number(key)), state);
}

/// \brief Converts a GLFW key into a signal::KEY.
///
/// \param key  GLFW key value.
///
/// \return The same key as signal::KEY.
KEY from_glfw_key(int key);

} // namespace signal
