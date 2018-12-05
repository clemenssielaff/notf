#pragma once

#include "notf/meta/time.hpp"
#include "notf/meta/types.hpp"

#include "notf/common/vector2.hpp"

NOTF_OPEN_NAMESPACE

// key ============================================================================================================== //

struct Key {

    // types ----------------------------------------------------------------------------------- //
public:
    enum class Action {
        PRESS,
        HOLD,
        RELEASE,
    };

    /// Modifier keys.
    /// If you hold down more than one key of the same modifier (both shift-keys, for example),
    /// the flag is still set only once (meaning there is no double-shift modifier).
    enum class Modifier {
        NONE = 0 << 1,
        SHIFT = 1 << 1,
        CTRL = 1 << 2,
        ALT = 1 << 3,
        SUPER = 1 << 4,
    };

    enum class Token {
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
        UNKNOWN = 255,
        __last = MENU,
    };

    /// Keyboard input.
    struct Stroke {
        const Token key;           /// Key pressed (as determined by GLFW)
        const int scancode;        /// Sytem scancode, use to identify keys not recognized by GLFW.
        const Modifier modifier;   /// Modifiers pressed while the stroke was generated.
        const duration_t duration; /// Time that the key remains pressed.
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (unknown key) Constructor.
    constexpr Key() = default;

    /// Value Constructor.
    /// @param glfw_key GLFW key token.
    /// @param modifier Active keyboard modifiers.
    /// @param scancode System-dependent scancode as determined by GLFW.
    Key(int glfw_key, Modifier modifier = Modifier::NONE, int scancode = 0);

    /// Value Constructor.
    /// The generated Key will correspond to the key required to generate the character on a US keyboard.
    /// For alphanumeric characters + "space" this will always generate the correct Key::Id, but all other symbols are
    /// layout-specific and may not match your system's layout.
    /// If the given character is uppercase, the "SHIFT" modifier is added automatically.
    /// @param character    ASCII character to match.
    /// @param modifier     Active keyboard modifiers.
    /// @param scancode     System-dependent scancode as determined by GLFW.
    Key(char character, Modifier modifier = Modifier::NONE, int scancode = 0);

    /// Converts this Key into the corresponding GLFW key token.
    int to_glfw_key() const;

    // fields --------------------------------------------------------------------------------- //
public:
    /// Key Token, corresponds to GLFW key tokens (which are plain integers).
    const Token token = Token::UNKNOWN;

    /// Modifier keys pressed during the key stroke.
    const Modifier modifier = Modifier::NONE;

    /// The key will be UNKNOWN if GLFW lacks a key token for it, for example E-mail and Play keys.
    /// The scancode on the other hand is unique for every key, regardless of whether it has a key token. Scancodes are
    /// platform-specific but consistent over time, so keys will have different scancodes depending on the platform but
    /// they are safe to save to disk.
    const int scancode = 0;
};

/// Key::Modifier arithmetic.
inline constexpr Key::Modifier operator+(const Key::Modifier lhs, const Key::Modifier rhs) noexcept {
    return Key::Modifier(to_number(lhs) | to_number(rhs));
}
inline constexpr Key::Modifier operator-(const Key::Modifier lhs, const Key::Modifier rhs) noexcept {
    return Key::Modifier(to_number(lhs) & ~to_number(rhs));
}

/// Convenience Key constructor
inline Key operator+(const char character, const Key::Modifier modifier) noexcept { return Key(character, modifier); };

// mouse ============================================================================================================ //

struct Mouse {

    // types ----------------------------------------------------------------------------------- //
public:
    enum class Action {
        PRESS,
        HOLD,
        SCROLL,
        RELEASE,
    };

    /// All mouse buttons recognized by GLFW.
    /// Can be used as indices for a ButtonStateSet object.
    enum class Button {
        __first = 0,
        BUTTON_1 = __first,
        BUTTON_2,
        BUTTON_3,
        BUTTON_4,
        BUTTON_5,
        BUTTON_6,
        BUTTON_7,
        BUTTON_8,
        NONE,
        LEFT = BUTTON_1,
        RIGHT = BUTTON_2,
        MIDDLE = BUTTON_3,
        INVALID = 255,
        __last = BUTTON_8,
    };

    struct Click {
        const Button button;
        const Key::Modifier modifier;
        const duration_t duration;
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default (no button) constructor.
    Mouse() = default;

    /// Value Constructor.
    /// @param button   Mouse button.
    /// @param x        X-axis position of the mouse in screen coordinates.
    /// @param y        Y-axis position of the mouse in screen coordinates.
    Mouse(Button button, int x = -1, int y = -1) : button(button), position{x, y} {}

    // fields --------------------------------------------------------------------------------- //
public:
    /// Mouse button.
    const Button button = Button::NONE;

    /// Position of the mouse in screen coordinates.
    /// Default is (-1, -1), meaning "unset".
    const V2i position{-1, -1};
};

NOTF_CLOSE_NAMESPACE
