#include "notf/app/input.hpp"

#include <locale>

#include "notf/meta/log.hpp"

#include "notf/common/string.hpp"

#include "notf/app/glfw.hpp"

// conversions ====================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

constexpr Key::Token id_from_glfw_key(int key) {
    switch (key) {
    case GLFW_KEY_SPACE: return Key::Token::SPACE;
    case GLFW_KEY_APOSTROPHE: return Key::Token::APOSTROPHE;
    case GLFW_KEY_COMMA: return Key::Token::COMMA;
    case GLFW_KEY_MINUS: return Key::Token::MINUS;
    case GLFW_KEY_PERIOD: return Key::Token::PERIOD;
    case GLFW_KEY_SLASH: return Key::Token::SLASH;
    case GLFW_KEY_0: return Key::Token::ZERO;
    case GLFW_KEY_1: return Key::Token::ONE;
    case GLFW_KEY_2: return Key::Token::TWO;
    case GLFW_KEY_3: return Key::Token::THREE;
    case GLFW_KEY_4: return Key::Token::FOUR;
    case GLFW_KEY_5: return Key::Token::FIVE;
    case GLFW_KEY_6: return Key::Token::SIX;
    case GLFW_KEY_7: return Key::Token::SEVEN;
    case GLFW_KEY_8: return Key::Token::EIGHT;
    case GLFW_KEY_9: return Key::Token::NINE;
    case GLFW_KEY_SEMICOLON: return Key::Token::SEMICOLON;
    case GLFW_KEY_EQUAL: return Key::Token::EQUAL;
    case GLFW_KEY_A: return Key::Token::A;
    case GLFW_KEY_B: return Key::Token::B;
    case GLFW_KEY_C: return Key::Token::C;
    case GLFW_KEY_D: return Key::Token::D;
    case GLFW_KEY_E: return Key::Token::E;
    case GLFW_KEY_F: return Key::Token::F;
    case GLFW_KEY_G: return Key::Token::G;
    case GLFW_KEY_H: return Key::Token::H;
    case GLFW_KEY_I: return Key::Token::I;
    case GLFW_KEY_J: return Key::Token::J;
    case GLFW_KEY_K: return Key::Token::K;
    case GLFW_KEY_L: return Key::Token::L;
    case GLFW_KEY_M: return Key::Token::M;
    case GLFW_KEY_N: return Key::Token::N;
    case GLFW_KEY_O: return Key::Token::O;
    case GLFW_KEY_P: return Key::Token::P;
    case GLFW_KEY_Q: return Key::Token::Q;
    case GLFW_KEY_R: return Key::Token::R;
    case GLFW_KEY_S: return Key::Token::S;
    case GLFW_KEY_T: return Key::Token::T;
    case GLFW_KEY_U: return Key::Token::U;
    case GLFW_KEY_V: return Key::Token::V;
    case GLFW_KEY_W: return Key::Token::W;
    case GLFW_KEY_X: return Key::Token::X;
    case GLFW_KEY_Y: return Key::Token::Y;
    case GLFW_KEY_Z: return Key::Token::Z;
    case GLFW_KEY_LEFT_BRACKET: return Key::Token::LEFT_BRACKET;
    case GLFW_KEY_BACKSLASH: return Key::Token::BACKSLASH;
    case GLFW_KEY_RIGHT_BRACKET: return Key::Token::RIGHT_BRACKET;
    case GLFW_KEY_GRAVE_ACCENT: return Key::Token::GRAVE_ACCENT;
    case GLFW_KEY_WORLD_1: return Key::Token::WORLD_1;
    case GLFW_KEY_WORLD_2: return Key::Token::WORLD_2;
    case GLFW_KEY_ESCAPE: return Key::Token::ESCAPE;
    case GLFW_KEY_ENTER: return Key::Token::ENTER;
    case GLFW_KEY_TAB: return Key::Token::TAB;
    case GLFW_KEY_BACKSPACE: return Key::Token::BACKSPACE;
    case GLFW_KEY_INSERT: return Key::Token::INSERT;
    case GLFW_KEY_DELETE: return Key::Token::DEL;
    case GLFW_KEY_RIGHT: return Key::Token::RIGHT;
    case GLFW_KEY_LEFT: return Key::Token::LEFT;
    case GLFW_KEY_DOWN: return Key::Token::DOWN;
    case GLFW_KEY_UP: return Key::Token::UP;
    case GLFW_KEY_PAGE_UP: return Key::Token::PAGE_UP;
    case GLFW_KEY_PAGE_DOWN: return Key::Token::PAGE_DOWN;
    case GLFW_KEY_HOME: return Key::Token::HOME;
    case GLFW_KEY_END: return Key::Token::END;
    case GLFW_KEY_CAPS_LOCK: return Key::Token::CAPS_LOCK;
    case GLFW_KEY_SCROLL_LOCK: return Key::Token::SCROLL_LOCK;
    case GLFW_KEY_NUM_LOCK: return Key::Token::NUM_LOCK;
    case GLFW_KEY_PRINT_SCREEN: return Key::Token::PRINT_SCREEN;
    case GLFW_KEY_PAUSE: return Key::Token::PAUSE;
    case GLFW_KEY_F1: return Key::Token::F1;
    case GLFW_KEY_F2: return Key::Token::F2;
    case GLFW_KEY_F3: return Key::Token::F3;
    case GLFW_KEY_F4: return Key::Token::F4;
    case GLFW_KEY_F5: return Key::Token::F5;
    case GLFW_KEY_F6: return Key::Token::F6;
    case GLFW_KEY_F7: return Key::Token::F7;
    case GLFW_KEY_F8: return Key::Token::F8;
    case GLFW_KEY_F9: return Key::Token::F9;
    case GLFW_KEY_F10: return Key::Token::F10;
    case GLFW_KEY_F11: return Key::Token::F11;
    case GLFW_KEY_F12: return Key::Token::F12;
    case GLFW_KEY_F13: return Key::Token::F13;
    case GLFW_KEY_F14: return Key::Token::F14;
    case GLFW_KEY_F15: return Key::Token::F15;
    case GLFW_KEY_F16: return Key::Token::F16;
    case GLFW_KEY_F17: return Key::Token::F17;
    case GLFW_KEY_F18: return Key::Token::F18;
    case GLFW_KEY_F19: return Key::Token::F19;
    case GLFW_KEY_F20: return Key::Token::F20;
    case GLFW_KEY_F21: return Key::Token::F21;
    case GLFW_KEY_F22: return Key::Token::F22;
    case GLFW_KEY_F23: return Key::Token::F23;
    case GLFW_KEY_F24: return Key::Token::F24;
    case GLFW_KEY_F25: return Key::Token::F25;
    case GLFW_KEY_KP_0: return Key::Token::KP_0;
    case GLFW_KEY_KP_1: return Key::Token::KP_1;
    case GLFW_KEY_KP_2: return Key::Token::KP_2;
    case GLFW_KEY_KP_3: return Key::Token::KP_3;
    case GLFW_KEY_KP_4: return Key::Token::KP_4;
    case GLFW_KEY_KP_5: return Key::Token::KP_5;
    case GLFW_KEY_KP_6: return Key::Token::KP_6;
    case GLFW_KEY_KP_7: return Key::Token::KP_7;
    case GLFW_KEY_KP_8: return Key::Token::KP_8;
    case GLFW_KEY_KP_9: return Key::Token::KP_9;
    case GLFW_KEY_KP_DECIMAL: return Key::Token::KP_DECIMAL;
    case GLFW_KEY_KP_DIVIDE: return Key::Token::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY: return Key::Token::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT: return Key::Token::KP_SUBTRACT;
    case GLFW_KEY_KP_ADD: return Key::Token::KP_ADD;
    case GLFW_KEY_KP_ENTER: return Key::Token::KP_ENTER;
    case GLFW_KEY_KP_EQUAL: return Key::Token::KP_EQUAL;
    case GLFW_KEY_LEFT_SHIFT: return Key::Token::LEFT_SHIFT;
    case GLFW_KEY_LEFT_CONTROL: return Key::Token::LEFT_CONTROL;
    case GLFW_KEY_LEFT_ALT: return Key::Token::LEFT_ALT;
    case GLFW_KEY_LEFT_SUPER: return Key::Token::LEFT_SUPER;
    case GLFW_KEY_RIGHT_SHIFT: return Key::Token::RIGHT_SHIFT;
    case GLFW_KEY_RIGHT_CONTROL: return Key::Token::RIGHT_CONTROL;
    case GLFW_KEY_RIGHT_ALT: return Key::Token::RIGHT_ALT;
    case GLFW_KEY_RIGHT_SUPER: return Key::Token::RIGHT_SUPER;
    case GLFW_KEY_MENU: return Key::Token::MENU;
    }
    NOTF_LOG_WARN("Requested invalid key: {}", key);
    return Key::Token::UNKNOWN;
}

constexpr Key::Token id_from_char(const char key) {
    switch (key) {
    case 'a': NOTF_FALLTHROUGH;
    case 'A': return Key::Token::A;
    case 'b': NOTF_FALLTHROUGH;
    case 'B': return Key::Token::B;
    case 'c': NOTF_FALLTHROUGH;
    case 'C': return Key::Token::C;
    case 'd': NOTF_FALLTHROUGH;
    case 'D': return Key::Token::D;
    case 'e': NOTF_FALLTHROUGH;
    case 'E': return Key::Token::E;
    case 'f': NOTF_FALLTHROUGH;
    case 'F': return Key::Token::F;
    case 'g': NOTF_FALLTHROUGH;
    case 'G': return Key::Token::G;
    case 'h': NOTF_FALLTHROUGH;
    case 'H': return Key::Token::H;
    case 'i': NOTF_FALLTHROUGH;
    case 'I': return Key::Token::I;
    case 'j': NOTF_FALLTHROUGH;
    case 'J': return Key::Token::J;
    case 'k': NOTF_FALLTHROUGH;
    case 'K': return Key::Token::K;
    case 'l': NOTF_FALLTHROUGH;
    case 'L': return Key::Token::L;
    case 'm': NOTF_FALLTHROUGH;
    case 'M': return Key::Token::M;
    case 'n': NOTF_FALLTHROUGH;
    case 'N': return Key::Token::N;
    case 'o': NOTF_FALLTHROUGH;
    case 'O': return Key::Token::O;
    case 'p': NOTF_FALLTHROUGH;
    case 'P': return Key::Token::P;
    case 'q': NOTF_FALLTHROUGH;
    case 'Q': return Key::Token::Q;
    case 'r': NOTF_FALLTHROUGH;
    case 'R': return Key::Token::R;
    case 's': NOTF_FALLTHROUGH;
    case 'S': return Key::Token::S;
    case 't': NOTF_FALLTHROUGH;
    case 'T': return Key::Token::T;
    case 'u': NOTF_FALLTHROUGH;
    case 'U': return Key::Token::U;
    case 'v': NOTF_FALLTHROUGH;
    case 'V': return Key::Token::V;
    case 'w': NOTF_FALLTHROUGH;
    case 'W': return Key::Token::W;
    case 'x': NOTF_FALLTHROUGH;
    case 'X': return Key::Token::X;
    case 'y': NOTF_FALLTHROUGH;
    case 'Y': return Key::Token::Y;
    case 'z': NOTF_FALLTHROUGH;
    case 'Z': return Key::Token::Z;
    case '!': NOTF_FALLTHROUGH;
    case '1': return Key::Token::ONE;
    case '@': NOTF_FALLTHROUGH;
    case '2': return Key::Token::TWO;
    case '#': NOTF_FALLTHROUGH;
    case '3': return Key::Token::THREE;
    case '$': NOTF_FALLTHROUGH;
    case '4': return Key::Token::FOUR;
    case '%': NOTF_FALLTHROUGH;
    case '5': return Key::Token::FIVE;
    case '^': NOTF_FALLTHROUGH;
    case '6': return Key::Token::SIX;
    case '&': NOTF_FALLTHROUGH;
    case '7': return Key::Token::SEVEN;
    case '*': NOTF_FALLTHROUGH;
    case '8': return Key::Token::EIGHT;
    case '(': NOTF_FALLTHROUGH;
    case '9': return Key::Token::NINE;
    case ')': NOTF_FALLTHROUGH;
    case '0': return Key::Token::ZERO;
    case ' ': return Key::Token::SPACE;
    case '"': NOTF_FALLTHROUGH;
    case '\'': return Key::Token::APOSTROPHE;
    case '<': NOTF_FALLTHROUGH;
    case ',': return Key::Token::COMMA;
    case '_': NOTF_FALLTHROUGH;
    case '-': return Key::Token::MINUS;
    case '>': NOTF_FALLTHROUGH;
    case '.': return Key::Token::PERIOD;
    case '?': NOTF_FALLTHROUGH;
    case '/': return Key::Token::SLASH;
    case ':': NOTF_FALLTHROUGH;
    case ';': return Key::Token::SEMICOLON;
    case '+': NOTF_FALLTHROUGH;
    case '=': return Key::Token::EQUAL;
    case '{': NOTF_FALLTHROUGH;
    case '[': return Key::Token::LEFT_BRACKET;
    case '|': NOTF_FALLTHROUGH;
    case '\\': return Key::Token::BACKSLASH;
    case '}': NOTF_FALLTHROUGH;
    case ']': return Key::Token::RIGHT_BRACKET;
    case '~': NOTF_FALLTHROUGH;
    case '`': return Key::Token::GRAVE_ACCENT;
    }
    NOTF_LOG_WARN("Requested unknown key: {}", key);
    return Key::Token::UNKNOWN;
}

} // namespace

NOTF_OPEN_NAMESPACE

// key ============================================================================================================== //

Key::Key(const int glfw_key, Modifier modifier, const int scancode)
    : token(id_from_glfw_key(glfw_key)), modifier(modifier), scancode(scancode) {}

Key::Key(const char character, Modifier modifier, const int scancode)
    : token(id_from_char(character))
    , modifier(modifier + (is_upper(character) ? Modifier::SHIFT : Modifier::NONE))
    , scancode(scancode) {}

int Key::to_glfw_key() const {
    switch (token) {
    case Key::Token::SPACE: return GLFW_KEY_SPACE;
    case Key::Token::APOSTROPHE: return GLFW_KEY_APOSTROPHE;
    case Key::Token::COMMA: return GLFW_KEY_COMMA;
    case Key::Token::MINUS: return GLFW_KEY_MINUS;
    case Key::Token::PERIOD: return GLFW_KEY_PERIOD;
    case Key::Token::SLASH: return GLFW_KEY_SLASH;
    case Key::Token::ZERO: return GLFW_KEY_0;
    case Key::Token::ONE: return GLFW_KEY_1;
    case Key::Token::TWO: return GLFW_KEY_2;
    case Key::Token::THREE: return GLFW_KEY_3;
    case Key::Token::FOUR: return GLFW_KEY_4;
    case Key::Token::FIVE: return GLFW_KEY_5;
    case Key::Token::SIX: return GLFW_KEY_6;
    case Key::Token::SEVEN: return GLFW_KEY_7;
    case Key::Token::EIGHT: return GLFW_KEY_8;
    case Key::Token::NINE: return GLFW_KEY_9;
    case Key::Token::SEMICOLON: return GLFW_KEY_SEMICOLON;
    case Key::Token::EQUAL: return GLFW_KEY_EQUAL;
    case Key::Token::A: return GLFW_KEY_A;
    case Key::Token::B: return GLFW_KEY_B;
    case Key::Token::C: return GLFW_KEY_C;
    case Key::Token::D: return GLFW_KEY_D;
    case Key::Token::E: return GLFW_KEY_E;
    case Key::Token::F: return GLFW_KEY_F;
    case Key::Token::G: return GLFW_KEY_G;
    case Key::Token::H: return GLFW_KEY_H;
    case Key::Token::I: return GLFW_KEY_I;
    case Key::Token::J: return GLFW_KEY_J;
    case Key::Token::K: return GLFW_KEY_K;
    case Key::Token::L: return GLFW_KEY_L;
    case Key::Token::M: return GLFW_KEY_M;
    case Key::Token::N: return GLFW_KEY_N;
    case Key::Token::O: return GLFW_KEY_O;
    case Key::Token::P: return GLFW_KEY_P;
    case Key::Token::Q: return GLFW_KEY_Q;
    case Key::Token::R: return GLFW_KEY_R;
    case Key::Token::S: return GLFW_KEY_S;
    case Key::Token::T: return GLFW_KEY_T;
    case Key::Token::U: return GLFW_KEY_U;
    case Key::Token::V: return GLFW_KEY_V;
    case Key::Token::W: return GLFW_KEY_W;
    case Key::Token::X: return GLFW_KEY_X;
    case Key::Token::Y: return GLFW_KEY_Y;
    case Key::Token::Z: return GLFW_KEY_Z;
    case Key::Token::LEFT_BRACKET: return GLFW_KEY_LEFT_BRACKET;
    case Key::Token::BACKSLASH: return GLFW_KEY_BACKSLASH;
    case Key::Token::RIGHT_BRACKET: return GLFW_KEY_RIGHT_BRACKET;
    case Key::Token::GRAVE_ACCENT: return GLFW_KEY_GRAVE_ACCENT;
    case Key::Token::WORLD_1: return GLFW_KEY_WORLD_1;
    case Key::Token::WORLD_2: return GLFW_KEY_WORLD_2;
    case Key::Token::ESCAPE: return GLFW_KEY_ESCAPE;
    case Key::Token::ENTER: return GLFW_KEY_ENTER;
    case Key::Token::TAB: return GLFW_KEY_TAB;
    case Key::Token::BACKSPACE: return GLFW_KEY_BACKSPACE;
    case Key::Token::INSERT: return GLFW_KEY_INSERT;
    case Key::Token::DEL: return GLFW_KEY_DELETE;
    case Key::Token::RIGHT: return GLFW_KEY_RIGHT;
    case Key::Token::LEFT: return GLFW_KEY_LEFT;
    case Key::Token::DOWN: return GLFW_KEY_DOWN;
    case Key::Token::UP: return GLFW_KEY_UP;
    case Key::Token::PAGE_UP: return GLFW_KEY_PAGE_UP;
    case Key::Token::PAGE_DOWN: return GLFW_KEY_PAGE_DOWN;
    case Key::Token::HOME: return GLFW_KEY_HOME;
    case Key::Token::END: return GLFW_KEY_END;
    case Key::Token::CAPS_LOCK: return GLFW_KEY_CAPS_LOCK;
    case Key::Token::SCROLL_LOCK: return GLFW_KEY_SCROLL_LOCK;
    case Key::Token::NUM_LOCK: return GLFW_KEY_NUM_LOCK;
    case Key::Token::PRINT_SCREEN: return GLFW_KEY_PRINT_SCREEN;
    case Key::Token::PAUSE: return GLFW_KEY_PAUSE;
    case Key::Token::F1: return GLFW_KEY_F1;
    case Key::Token::F2: return GLFW_KEY_F2;
    case Key::Token::F3: return GLFW_KEY_F3;
    case Key::Token::F4: return GLFW_KEY_F4;
    case Key::Token::F5: return GLFW_KEY_F5;
    case Key::Token::F6: return GLFW_KEY_F6;
    case Key::Token::F7: return GLFW_KEY_F7;
    case Key::Token::F8: return GLFW_KEY_F8;
    case Key::Token::F9: return GLFW_KEY_F9;
    case Key::Token::F10: return GLFW_KEY_F10;
    case Key::Token::F11: return GLFW_KEY_F11;
    case Key::Token::F12: return GLFW_KEY_F12;
    case Key::Token::F13: return GLFW_KEY_F13;
    case Key::Token::F14: return GLFW_KEY_F14;
    case Key::Token::F15: return GLFW_KEY_F15;
    case Key::Token::F16: return GLFW_KEY_F16;
    case Key::Token::F17: return GLFW_KEY_F17;
    case Key::Token::F18: return GLFW_KEY_F18;
    case Key::Token::F19: return GLFW_KEY_F19;
    case Key::Token::F20: return GLFW_KEY_F20;
    case Key::Token::F21: return GLFW_KEY_F21;
    case Key::Token::F22: return GLFW_KEY_F22;
    case Key::Token::F23: return GLFW_KEY_F23;
    case Key::Token::F24: return GLFW_KEY_F24;
    case Key::Token::F25: return GLFW_KEY_F25;
    case Key::Token::KP_0: return GLFW_KEY_KP_0;
    case Key::Token::KP_1: return GLFW_KEY_KP_1;
    case Key::Token::KP_2: return GLFW_KEY_KP_2;
    case Key::Token::KP_3: return GLFW_KEY_KP_3;
    case Key::Token::KP_4: return GLFW_KEY_KP_4;
    case Key::Token::KP_5: return GLFW_KEY_KP_5;
    case Key::Token::KP_6: return GLFW_KEY_KP_6;
    case Key::Token::KP_7: return GLFW_KEY_KP_7;
    case Key::Token::KP_8: return GLFW_KEY_KP_8;
    case Key::Token::KP_9: return GLFW_KEY_KP_9;
    case Key::Token::KP_DECIMAL: return GLFW_KEY_KP_DECIMAL;
    case Key::Token::KP_DIVIDE: return GLFW_KEY_KP_DIVIDE;
    case Key::Token::KP_MULTIPLY: return GLFW_KEY_KP_MULTIPLY;
    case Key::Token::KP_SUBTRACT: return GLFW_KEY_KP_SUBTRACT;
    case Key::Token::KP_ADD: return GLFW_KEY_KP_ADD;
    case Key::Token::KP_ENTER: return GLFW_KEY_KP_ENTER;
    case Key::Token::KP_EQUAL: return GLFW_KEY_KP_EQUAL;
    case Key::Token::LEFT_SHIFT: return GLFW_KEY_LEFT_SHIFT;
    case Key::Token::LEFT_CONTROL: return GLFW_KEY_LEFT_CONTROL;
    case Key::Token::LEFT_ALT: return GLFW_KEY_LEFT_ALT;
    case Key::Token::LEFT_SUPER: return GLFW_KEY_LEFT_SUPER;
    case Key::Token::RIGHT_SHIFT: return GLFW_KEY_RIGHT_SHIFT;
    case Key::Token::RIGHT_CONTROL: return GLFW_KEY_RIGHT_CONTROL;
    case Key::Token::RIGHT_ALT: return GLFW_KEY_RIGHT_ALT;
    case Key::Token::RIGHT_SUPER: return GLFW_KEY_RIGHT_SUPER;
    case Key::Token::MENU: return GLFW_KEY_MENU;
    case Key::Token::UNKNOWN: return GLFW_KEY_UNKNOWN;
    }
    return GLFW_KEY_UNKNOWN;
}

NOTF_CLOSE_NAMESPACE
