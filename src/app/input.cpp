#include "notf/app/input.hpp"

#include <locale>

#include "notf/meta/log.hpp"

#include "notf/common/string.hpp"

#include "notf/graphic/glfw.hpp"

// conversions ====================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

constexpr KeyInput::Token id_from_glfw_key(int key) {
    switch (key) {
    case GLFW_KEY_SPACE: return KeyInput::Token::SPACE;
    case GLFW_KEY_APOSTROPHE: return KeyInput::Token::APOSTROPHE;
    case GLFW_KEY_COMMA: return KeyInput::Token::COMMA;
    case GLFW_KEY_MINUS: return KeyInput::Token::MINUS;
    case GLFW_KEY_PERIOD: return KeyInput::Token::PERIOD;
    case GLFW_KEY_SLASH: return KeyInput::Token::SLASH;
    case GLFW_KEY_0: return KeyInput::Token::ZERO;
    case GLFW_KEY_1: return KeyInput::Token::ONE;
    case GLFW_KEY_2: return KeyInput::Token::TWO;
    case GLFW_KEY_3: return KeyInput::Token::THREE;
    case GLFW_KEY_4: return KeyInput::Token::FOUR;
    case GLFW_KEY_5: return KeyInput::Token::FIVE;
    case GLFW_KEY_6: return KeyInput::Token::SIX;
    case GLFW_KEY_7: return KeyInput::Token::SEVEN;
    case GLFW_KEY_8: return KeyInput::Token::EIGHT;
    case GLFW_KEY_9: return KeyInput::Token::NINE;
    case GLFW_KEY_SEMICOLON: return KeyInput::Token::SEMICOLON;
    case GLFW_KEY_EQUAL: return KeyInput::Token::EQUAL;
    case GLFW_KEY_A: return KeyInput::Token::A;
    case GLFW_KEY_B: return KeyInput::Token::B;
    case GLFW_KEY_C: return KeyInput::Token::C;
    case GLFW_KEY_D: return KeyInput::Token::D;
    case GLFW_KEY_E: return KeyInput::Token::E;
    case GLFW_KEY_F: return KeyInput::Token::F;
    case GLFW_KEY_G: return KeyInput::Token::G;
    case GLFW_KEY_H: return KeyInput::Token::H;
    case GLFW_KEY_I: return KeyInput::Token::I;
    case GLFW_KEY_J: return KeyInput::Token::J;
    case GLFW_KEY_K: return KeyInput::Token::K;
    case GLFW_KEY_L: return KeyInput::Token::L;
    case GLFW_KEY_M: return KeyInput::Token::M;
    case GLFW_KEY_N: return KeyInput::Token::N;
    case GLFW_KEY_O: return KeyInput::Token::O;
    case GLFW_KEY_P: return KeyInput::Token::P;
    case GLFW_KEY_Q: return KeyInput::Token::Q;
    case GLFW_KEY_R: return KeyInput::Token::R;
    case GLFW_KEY_S: return KeyInput::Token::S;
    case GLFW_KEY_T: return KeyInput::Token::T;
    case GLFW_KEY_U: return KeyInput::Token::U;
    case GLFW_KEY_V: return KeyInput::Token::V;
    case GLFW_KEY_W: return KeyInput::Token::W;
    case GLFW_KEY_X: return KeyInput::Token::X;
    case GLFW_KEY_Y: return KeyInput::Token::Y;
    case GLFW_KEY_Z: return KeyInput::Token::Z;
    case GLFW_KEY_LEFT_BRACKET: return KeyInput::Token::LEFT_BRACKET;
    case GLFW_KEY_BACKSLASH: return KeyInput::Token::BACKSLASH;
    case GLFW_KEY_RIGHT_BRACKET: return KeyInput::Token::RIGHT_BRACKET;
    case GLFW_KEY_GRAVE_ACCENT: return KeyInput::Token::GRAVE_ACCENT;
    case GLFW_KEY_WORLD_1: return KeyInput::Token::WORLD_1;
    case GLFW_KEY_WORLD_2: return KeyInput::Token::WORLD_2;
    case GLFW_KEY_ESCAPE: return KeyInput::Token::ESCAPE;
    case GLFW_KEY_ENTER: return KeyInput::Token::ENTER;
    case GLFW_KEY_TAB: return KeyInput::Token::TAB;
    case GLFW_KEY_BACKSPACE: return KeyInput::Token::BACKSPACE;
    case GLFW_KEY_INSERT: return KeyInput::Token::INSERT;
    case GLFW_KEY_DELETE: return KeyInput::Token::DEL;
    case GLFW_KEY_RIGHT: return KeyInput::Token::RIGHT;
    case GLFW_KEY_LEFT: return KeyInput::Token::LEFT;
    case GLFW_KEY_DOWN: return KeyInput::Token::DOWN;
    case GLFW_KEY_UP: return KeyInput::Token::UP;
    case GLFW_KEY_PAGE_UP: return KeyInput::Token::PAGE_UP;
    case GLFW_KEY_PAGE_DOWN: return KeyInput::Token::PAGE_DOWN;
    case GLFW_KEY_HOME: return KeyInput::Token::HOME;
    case GLFW_KEY_END: return KeyInput::Token::END;
    case GLFW_KEY_CAPS_LOCK: return KeyInput::Token::CAPS_LOCK;
    case GLFW_KEY_SCROLL_LOCK: return KeyInput::Token::SCROLL_LOCK;
    case GLFW_KEY_NUM_LOCK: return KeyInput::Token::NUM_LOCK;
    case GLFW_KEY_PRINT_SCREEN: return KeyInput::Token::PRINT_SCREEN;
    case GLFW_KEY_PAUSE: return KeyInput::Token::PAUSE;
    case GLFW_KEY_F1: return KeyInput::Token::F1;
    case GLFW_KEY_F2: return KeyInput::Token::F2;
    case GLFW_KEY_F3: return KeyInput::Token::F3;
    case GLFW_KEY_F4: return KeyInput::Token::F4;
    case GLFW_KEY_F5: return KeyInput::Token::F5;
    case GLFW_KEY_F6: return KeyInput::Token::F6;
    case GLFW_KEY_F7: return KeyInput::Token::F7;
    case GLFW_KEY_F8: return KeyInput::Token::F8;
    case GLFW_KEY_F9: return KeyInput::Token::F9;
    case GLFW_KEY_F10: return KeyInput::Token::F10;
    case GLFW_KEY_F11: return KeyInput::Token::F11;
    case GLFW_KEY_F12: return KeyInput::Token::F12;
    case GLFW_KEY_F13: return KeyInput::Token::F13;
    case GLFW_KEY_F14: return KeyInput::Token::F14;
    case GLFW_KEY_F15: return KeyInput::Token::F15;
    case GLFW_KEY_F16: return KeyInput::Token::F16;
    case GLFW_KEY_F17: return KeyInput::Token::F17;
    case GLFW_KEY_F18: return KeyInput::Token::F18;
    case GLFW_KEY_F19: return KeyInput::Token::F19;
    case GLFW_KEY_F20: return KeyInput::Token::F20;
    case GLFW_KEY_F21: return KeyInput::Token::F21;
    case GLFW_KEY_F22: return KeyInput::Token::F22;
    case GLFW_KEY_F23: return KeyInput::Token::F23;
    case GLFW_KEY_F24: return KeyInput::Token::F24;
    case GLFW_KEY_F25: return KeyInput::Token::F25;
    case GLFW_KEY_KP_0: return KeyInput::Token::KP_0;
    case GLFW_KEY_KP_1: return KeyInput::Token::KP_1;
    case GLFW_KEY_KP_2: return KeyInput::Token::KP_2;
    case GLFW_KEY_KP_3: return KeyInput::Token::KP_3;
    case GLFW_KEY_KP_4: return KeyInput::Token::KP_4;
    case GLFW_KEY_KP_5: return KeyInput::Token::KP_5;
    case GLFW_KEY_KP_6: return KeyInput::Token::KP_6;
    case GLFW_KEY_KP_7: return KeyInput::Token::KP_7;
    case GLFW_KEY_KP_8: return KeyInput::Token::KP_8;
    case GLFW_KEY_KP_9: return KeyInput::Token::KP_9;
    case GLFW_KEY_KP_DECIMAL: return KeyInput::Token::KP_DECIMAL;
    case GLFW_KEY_KP_DIVIDE: return KeyInput::Token::KP_DIVIDE;
    case GLFW_KEY_KP_MULTIPLY: return KeyInput::Token::KP_MULTIPLY;
    case GLFW_KEY_KP_SUBTRACT: return KeyInput::Token::KP_SUBTRACT;
    case GLFW_KEY_KP_ADD: return KeyInput::Token::KP_ADD;
    case GLFW_KEY_KP_ENTER: return KeyInput::Token::KP_ENTER;
    case GLFW_KEY_KP_EQUAL: return KeyInput::Token::KP_EQUAL;
    case GLFW_KEY_LEFT_SHIFT: return KeyInput::Token::LEFT_SHIFT;
    case GLFW_KEY_LEFT_CONTROL: return KeyInput::Token::LEFT_CONTROL;
    case GLFW_KEY_LEFT_ALT: return KeyInput::Token::LEFT_ALT;
    case GLFW_KEY_LEFT_SUPER: return KeyInput::Token::LEFT_SUPER;
    case GLFW_KEY_RIGHT_SHIFT: return KeyInput::Token::RIGHT_SHIFT;
    case GLFW_KEY_RIGHT_CONTROL: return KeyInput::Token::RIGHT_CONTROL;
    case GLFW_KEY_RIGHT_ALT: return KeyInput::Token::RIGHT_ALT;
    case GLFW_KEY_RIGHT_SUPER: return KeyInput::Token::RIGHT_SUPER;
    case GLFW_KEY_MENU: return KeyInput::Token::MENU;
    }
    NOTF_LOG_WARN("Requested invalid key: {}", key);
    return KeyInput::Token::UNKNOWN;
}

constexpr KeyInput::Token id_from_char(const char key) {
    switch (key) {
    case 'a': NOTF_FALLTHROUGH;
    case 'A': return KeyInput::Token::A;
    case 'b': NOTF_FALLTHROUGH;
    case 'B': return KeyInput::Token::B;
    case 'c': NOTF_FALLTHROUGH;
    case 'C': return KeyInput::Token::C;
    case 'd': NOTF_FALLTHROUGH;
    case 'D': return KeyInput::Token::D;
    case 'e': NOTF_FALLTHROUGH;
    case 'E': return KeyInput::Token::E;
    case 'f': NOTF_FALLTHROUGH;
    case 'F': return KeyInput::Token::F;
    case 'g': NOTF_FALLTHROUGH;
    case 'G': return KeyInput::Token::G;
    case 'h': NOTF_FALLTHROUGH;
    case 'H': return KeyInput::Token::H;
    case 'i': NOTF_FALLTHROUGH;
    case 'I': return KeyInput::Token::I;
    case 'j': NOTF_FALLTHROUGH;
    case 'J': return KeyInput::Token::J;
    case 'k': NOTF_FALLTHROUGH;
    case 'K': return KeyInput::Token::K;
    case 'l': NOTF_FALLTHROUGH;
    case 'L': return KeyInput::Token::L;
    case 'm': NOTF_FALLTHROUGH;
    case 'M': return KeyInput::Token::M;
    case 'n': NOTF_FALLTHROUGH;
    case 'N': return KeyInput::Token::N;
    case 'o': NOTF_FALLTHROUGH;
    case 'O': return KeyInput::Token::O;
    case 'p': NOTF_FALLTHROUGH;
    case 'P': return KeyInput::Token::P;
    case 'q': NOTF_FALLTHROUGH;
    case 'Q': return KeyInput::Token::Q;
    case 'r': NOTF_FALLTHROUGH;
    case 'R': return KeyInput::Token::R;
    case 's': NOTF_FALLTHROUGH;
    case 'S': return KeyInput::Token::S;
    case 't': NOTF_FALLTHROUGH;
    case 'T': return KeyInput::Token::T;
    case 'u': NOTF_FALLTHROUGH;
    case 'U': return KeyInput::Token::U;
    case 'v': NOTF_FALLTHROUGH;
    case 'V': return KeyInput::Token::V;
    case 'w': NOTF_FALLTHROUGH;
    case 'W': return KeyInput::Token::W;
    case 'x': NOTF_FALLTHROUGH;
    case 'X': return KeyInput::Token::X;
    case 'y': NOTF_FALLTHROUGH;
    case 'Y': return KeyInput::Token::Y;
    case 'z': NOTF_FALLTHROUGH;
    case 'Z': return KeyInput::Token::Z;
    case '!': NOTF_FALLTHROUGH;
    case '1': return KeyInput::Token::ONE;
    case '@': NOTF_FALLTHROUGH;
    case '2': return KeyInput::Token::TWO;
    case '#': NOTF_FALLTHROUGH;
    case '3': return KeyInput::Token::THREE;
    case '$': NOTF_FALLTHROUGH;
    case '4': return KeyInput::Token::FOUR;
    case '%': NOTF_FALLTHROUGH;
    case '5': return KeyInput::Token::FIVE;
    case '^': NOTF_FALLTHROUGH;
    case '6': return KeyInput::Token::SIX;
    case '&': NOTF_FALLTHROUGH;
    case '7': return KeyInput::Token::SEVEN;
    case '*': NOTF_FALLTHROUGH;
    case '8': return KeyInput::Token::EIGHT;
    case '(': NOTF_FALLTHROUGH;
    case '9': return KeyInput::Token::NINE;
    case ')': NOTF_FALLTHROUGH;
    case '0': return KeyInput::Token::ZERO;
    case ' ': return KeyInput::Token::SPACE;
    case '"': NOTF_FALLTHROUGH;
    case '\'': return KeyInput::Token::APOSTROPHE;
    case '<': NOTF_FALLTHROUGH;
    case ',': return KeyInput::Token::COMMA;
    case '_': NOTF_FALLTHROUGH;
    case '-': return KeyInput::Token::MINUS;
    case '>': NOTF_FALLTHROUGH;
    case '.': return KeyInput::Token::PERIOD;
    case '?': NOTF_FALLTHROUGH;
    case '/': return KeyInput::Token::SLASH;
    case ':': NOTF_FALLTHROUGH;
    case ';': return KeyInput::Token::SEMICOLON;
    case '+': NOTF_FALLTHROUGH;
    case '=': return KeyInput::Token::EQUAL;
    case '{': NOTF_FALLTHROUGH;
    case '[': return KeyInput::Token::LEFT_BRACKET;
    case '|': NOTF_FALLTHROUGH;
    case '\\': return KeyInput::Token::BACKSLASH;
    case '}': NOTF_FALLTHROUGH;
    case ']': return KeyInput::Token::RIGHT_BRACKET;
    case '~': NOTF_FALLTHROUGH;
    case '`': return KeyInput::Token::GRAVE_ACCENT;
    }
    NOTF_LOG_WARN("Requested unknown key: {}", key);
    return KeyInput::Token::UNKNOWN;
}

} // namespace

NOTF_OPEN_NAMESPACE

// key ============================================================================================================== //

KeyInput::KeyInput(const int glfw_key, Modifier modifier, const int scancode)
    : token(id_from_glfw_key(glfw_key)), modifier(modifier), scancode(scancode) {}

KeyInput::KeyInput(const char character, Modifier modifier, const int scancode)
    : token(id_from_char(character))
    , modifier(modifier + (is_upper(character) ? Modifier::SHIFT : Modifier::NONE))
    , scancode(scancode) {}

int KeyInput::to_glfw_key() const {
    switch (token) {
    case KeyInput::Token::SPACE: return GLFW_KEY_SPACE;
    case KeyInput::Token::APOSTROPHE: return GLFW_KEY_APOSTROPHE;
    case KeyInput::Token::COMMA: return GLFW_KEY_COMMA;
    case KeyInput::Token::MINUS: return GLFW_KEY_MINUS;
    case KeyInput::Token::PERIOD: return GLFW_KEY_PERIOD;
    case KeyInput::Token::SLASH: return GLFW_KEY_SLASH;
    case KeyInput::Token::ZERO: return GLFW_KEY_0;
    case KeyInput::Token::ONE: return GLFW_KEY_1;
    case KeyInput::Token::TWO: return GLFW_KEY_2;
    case KeyInput::Token::THREE: return GLFW_KEY_3;
    case KeyInput::Token::FOUR: return GLFW_KEY_4;
    case KeyInput::Token::FIVE: return GLFW_KEY_5;
    case KeyInput::Token::SIX: return GLFW_KEY_6;
    case KeyInput::Token::SEVEN: return GLFW_KEY_7;
    case KeyInput::Token::EIGHT: return GLFW_KEY_8;
    case KeyInput::Token::NINE: return GLFW_KEY_9;
    case KeyInput::Token::SEMICOLON: return GLFW_KEY_SEMICOLON;
    case KeyInput::Token::EQUAL: return GLFW_KEY_EQUAL;
    case KeyInput::Token::A: return GLFW_KEY_A;
    case KeyInput::Token::B: return GLFW_KEY_B;
    case KeyInput::Token::C: return GLFW_KEY_C;
    case KeyInput::Token::D: return GLFW_KEY_D;
    case KeyInput::Token::E: return GLFW_KEY_E;
    case KeyInput::Token::F: return GLFW_KEY_F;
    case KeyInput::Token::G: return GLFW_KEY_G;
    case KeyInput::Token::H: return GLFW_KEY_H;
    case KeyInput::Token::I: return GLFW_KEY_I;
    case KeyInput::Token::J: return GLFW_KEY_J;
    case KeyInput::Token::K: return GLFW_KEY_K;
    case KeyInput::Token::L: return GLFW_KEY_L;
    case KeyInput::Token::M: return GLFW_KEY_M;
    case KeyInput::Token::N: return GLFW_KEY_N;
    case KeyInput::Token::O: return GLFW_KEY_O;
    case KeyInput::Token::P: return GLFW_KEY_P;
    case KeyInput::Token::Q: return GLFW_KEY_Q;
    case KeyInput::Token::R: return GLFW_KEY_R;
    case KeyInput::Token::S: return GLFW_KEY_S;
    case KeyInput::Token::T: return GLFW_KEY_T;
    case KeyInput::Token::U: return GLFW_KEY_U;
    case KeyInput::Token::V: return GLFW_KEY_V;
    case KeyInput::Token::W: return GLFW_KEY_W;
    case KeyInput::Token::X: return GLFW_KEY_X;
    case KeyInput::Token::Y: return GLFW_KEY_Y;
    case KeyInput::Token::Z: return GLFW_KEY_Z;
    case KeyInput::Token::LEFT_BRACKET: return GLFW_KEY_LEFT_BRACKET;
    case KeyInput::Token::BACKSLASH: return GLFW_KEY_BACKSLASH;
    case KeyInput::Token::RIGHT_BRACKET: return GLFW_KEY_RIGHT_BRACKET;
    case KeyInput::Token::GRAVE_ACCENT: return GLFW_KEY_GRAVE_ACCENT;
    case KeyInput::Token::WORLD_1: return GLFW_KEY_WORLD_1;
    case KeyInput::Token::WORLD_2: return GLFW_KEY_WORLD_2;
    case KeyInput::Token::ESCAPE: return GLFW_KEY_ESCAPE;
    case KeyInput::Token::ENTER: return GLFW_KEY_ENTER;
    case KeyInput::Token::TAB: return GLFW_KEY_TAB;
    case KeyInput::Token::BACKSPACE: return GLFW_KEY_BACKSPACE;
    case KeyInput::Token::INSERT: return GLFW_KEY_INSERT;
    case KeyInput::Token::DEL: return GLFW_KEY_DELETE;
    case KeyInput::Token::RIGHT: return GLFW_KEY_RIGHT;
    case KeyInput::Token::LEFT: return GLFW_KEY_LEFT;
    case KeyInput::Token::DOWN: return GLFW_KEY_DOWN;
    case KeyInput::Token::UP: return GLFW_KEY_UP;
    case KeyInput::Token::PAGE_UP: return GLFW_KEY_PAGE_UP;
    case KeyInput::Token::PAGE_DOWN: return GLFW_KEY_PAGE_DOWN;
    case KeyInput::Token::HOME: return GLFW_KEY_HOME;
    case KeyInput::Token::END: return GLFW_KEY_END;
    case KeyInput::Token::CAPS_LOCK: return GLFW_KEY_CAPS_LOCK;
    case KeyInput::Token::SCROLL_LOCK: return GLFW_KEY_SCROLL_LOCK;
    case KeyInput::Token::NUM_LOCK: return GLFW_KEY_NUM_LOCK;
    case KeyInput::Token::PRINT_SCREEN: return GLFW_KEY_PRINT_SCREEN;
    case KeyInput::Token::PAUSE: return GLFW_KEY_PAUSE;
    case KeyInput::Token::F1: return GLFW_KEY_F1;
    case KeyInput::Token::F2: return GLFW_KEY_F2;
    case KeyInput::Token::F3: return GLFW_KEY_F3;
    case KeyInput::Token::F4: return GLFW_KEY_F4;
    case KeyInput::Token::F5: return GLFW_KEY_F5;
    case KeyInput::Token::F6: return GLFW_KEY_F6;
    case KeyInput::Token::F7: return GLFW_KEY_F7;
    case KeyInput::Token::F8: return GLFW_KEY_F8;
    case KeyInput::Token::F9: return GLFW_KEY_F9;
    case KeyInput::Token::F10: return GLFW_KEY_F10;
    case KeyInput::Token::F11: return GLFW_KEY_F11;
    case KeyInput::Token::F12: return GLFW_KEY_F12;
    case KeyInput::Token::F13: return GLFW_KEY_F13;
    case KeyInput::Token::F14: return GLFW_KEY_F14;
    case KeyInput::Token::F15: return GLFW_KEY_F15;
    case KeyInput::Token::F16: return GLFW_KEY_F16;
    case KeyInput::Token::F17: return GLFW_KEY_F17;
    case KeyInput::Token::F18: return GLFW_KEY_F18;
    case KeyInput::Token::F19: return GLFW_KEY_F19;
    case KeyInput::Token::F20: return GLFW_KEY_F20;
    case KeyInput::Token::F21: return GLFW_KEY_F21;
    case KeyInput::Token::F22: return GLFW_KEY_F22;
    case KeyInput::Token::F23: return GLFW_KEY_F23;
    case KeyInput::Token::F24: return GLFW_KEY_F24;
    case KeyInput::Token::F25: return GLFW_KEY_F25;
    case KeyInput::Token::KP_0: return GLFW_KEY_KP_0;
    case KeyInput::Token::KP_1: return GLFW_KEY_KP_1;
    case KeyInput::Token::KP_2: return GLFW_KEY_KP_2;
    case KeyInput::Token::KP_3: return GLFW_KEY_KP_3;
    case KeyInput::Token::KP_4: return GLFW_KEY_KP_4;
    case KeyInput::Token::KP_5: return GLFW_KEY_KP_5;
    case KeyInput::Token::KP_6: return GLFW_KEY_KP_6;
    case KeyInput::Token::KP_7: return GLFW_KEY_KP_7;
    case KeyInput::Token::KP_8: return GLFW_KEY_KP_8;
    case KeyInput::Token::KP_9: return GLFW_KEY_KP_9;
    case KeyInput::Token::KP_DECIMAL: return GLFW_KEY_KP_DECIMAL;
    case KeyInput::Token::KP_DIVIDE: return GLFW_KEY_KP_DIVIDE;
    case KeyInput::Token::KP_MULTIPLY: return GLFW_KEY_KP_MULTIPLY;
    case KeyInput::Token::KP_SUBTRACT: return GLFW_KEY_KP_SUBTRACT;
    case KeyInput::Token::KP_ADD: return GLFW_KEY_KP_ADD;
    case KeyInput::Token::KP_ENTER: return GLFW_KEY_KP_ENTER;
    case KeyInput::Token::KP_EQUAL: return GLFW_KEY_KP_EQUAL;
    case KeyInput::Token::LEFT_SHIFT: return GLFW_KEY_LEFT_SHIFT;
    case KeyInput::Token::LEFT_CONTROL: return GLFW_KEY_LEFT_CONTROL;
    case KeyInput::Token::LEFT_ALT: return GLFW_KEY_LEFT_ALT;
    case KeyInput::Token::LEFT_SUPER: return GLFW_KEY_LEFT_SUPER;
    case KeyInput::Token::RIGHT_SHIFT: return GLFW_KEY_RIGHT_SHIFT;
    case KeyInput::Token::RIGHT_CONTROL: return GLFW_KEY_RIGHT_CONTROL;
    case KeyInput::Token::RIGHT_ALT: return GLFW_KEY_RIGHT_ALT;
    case KeyInput::Token::RIGHT_SUPER: return GLFW_KEY_RIGHT_SUPER;
    case KeyInput::Token::MENU: return GLFW_KEY_MENU;
    case KeyInput::Token::UNKNOWN: NOTF_FALLTHROUGH;
    default: return GLFW_KEY_UNKNOWN;
    }
}

NOTF_CLOSE_NAMESPACE
