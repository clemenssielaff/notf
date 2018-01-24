#pragma once

#include <bitset>
#include <cassert>

#include "common/enum.hpp"

namespace notf {

/** All keys recognized by GLFW.
 * Can be used as indices for a KeyStateSet object.
 */
enum class Key : unsigned char {
    __first = 0,
    SPACE   = __first,
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
    INVALID = 255,
    __last  = MENU,
};

/** All mouse buttons recognized by GLFW.
 * Can be used as indices for a ButtonStateSet object.
 */
enum class Button : unsigned char {
    __first  = 0,
    BUTTON_1 = __first,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7,
    BUTTON_8,
    NONE,
    LEFT    = BUTTON_1,
    RIGHT   = BUTTON_2,
    MIDDLE  = BUTTON_3,
    INVALID = 255,
    __last  = BUTTON_8,
};

/** Actions you can do with a key. */
enum class KeyAction : unsigned char {
    RELEASE = 0,
    PRESS,
    REPEAT,
};

/** Actions you can do with a mouse. */
enum class MouseAction : unsigned char {
    RELEASE = 0,
    PRESS,
    MOVE,
    SCROLL,
};

/** Modifier keys.
 * If you hold down more than one key of the same modifier (both shift-keys, for example),
 * the flag is still set only once (meaning there is no double-shift modifier).
 */
enum KeyModifiers : unsigned char {
    NONE  = 0,
    SHIFT = 1,
    CTRL  = 2,
    ALT   = 4,
    SUPER = 8,
};

/** Converts a GLFW key to a notf::Key. */
Key from_glfw_key(int key);

/** The state of all regonized keys in a compact bitset.
 * True means pressed, false unpressed.
 * Use Key enum values as index to access individual key states.
 */
using KeyStateSet = std::bitset<to_number(Key::__last)>;

/** Checks the state of a given key in the KeyStateSet.
 * @param state_set KeyStateSet to test.
 * @param key       Key to test.
 * @return          True iff the key is pressed, false otherwise.
 */
inline bool test_key(const KeyStateSet& state_set, Key key)
{
    assert(key >= Key::__first && key <= Key::__last);
    return state_set.test(static_cast<size_t>(to_number(key)));
}

/** Sets the state of a given key in the KeyStateSet.
 * @param state_set    KeyStateSet to change.
 * @param key          Key to modify.
 * @param pressed      Whether the key is pressed or not.
 */
inline void set_key(KeyStateSet& state_set, Key key, bool pressed)
{
    assert(key >= Key::__first && key <= Key::__last);
    state_set.set(static_cast<size_t>(to_number(key)), pressed);
}

/** The state of all regonized buttons in a compact bitset.
 * True means pressed, false unpressed.
 * Use Button enum values as index to access individual button states.
 */
using ButtonStateSet = std::bitset<to_number(Button::__last)>;

/** Checks the state of a given button in the ButtonStateSet.
 * @param state_set ButtonStateSet to test.
 * @param button    Button to test.
 * @return          True iff the button is pressed, false otherwise.
 */
inline bool test_button(const ButtonStateSet& state_set, Button button)
{
    assert(button >= Button::__first && button <= Button::__last);
    return state_set.test(static_cast<size_t>(to_number(button)));
}

/** Sets the state of a given button in the ButtonStateSet.
 * @param state_set    ButtonStateSet to change.
 * @param button       Button to modify.
 * @param pressed      Whether the button is pressed or not.
 */
inline void set_button(ButtonStateSet& state_set, Button button, bool pressed)
{
    assert(button >= Button::__first && button <= Button::__last);
    state_set.set(static_cast<size_t>(to_number(button)), pressed);
}

/** Things the focus can do. */
enum class FocusAction : unsigned char {
    LOST = 0,
    GAINED,
};

} // namespace notf
