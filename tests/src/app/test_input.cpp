#include "catch.hpp"

#include "notf/graphic/glfw.hpp"

#include "notf/app/input.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("User Input Types", "[app][input]") {

    SECTION("keyboard input") {
        auto key = KeyInput('c', KeyInput::Modifier::SHIFT, 23);
        REQUIRE(key.token == KeyInput::Token::C);
        REQUIRE(key.modifier == KeyInput::Modifier::SHIFT);
        REQUIRE(key.scancode == 23);
    }

    SECTION("mouse input") {
        REQUIRE(MouseInput(MouseInput::Button::LEFT, V2i{50, 50}) == MouseInput(MouseInput::Button::LEFT, 50, 50));
    }

    SECTION("modifier arithmetic") {
        auto modifier = KeyInput::Modifier::NONE;
        REQUIRE(modifier == KeyInput::Modifier::NONE);

        modifier = KeyInput::Modifier::NONE + KeyInput::Modifier::ALT;
        REQUIRE(modifier == KeyInput::Modifier::ALT);

        modifier = KeyInput::Modifier::SHIFT + KeyInput::Modifier::ALT;
        REQUIRE((to_number(modifier) & to_number(KeyInput::Modifier::SHIFT)));
        REQUIRE((to_number(modifier) & to_number(KeyInput::Modifier::ALT)));

        modifier = modifier - KeyInput::Modifier::SHIFT;
        REQUIRE(modifier == KeyInput::Modifier::ALT);
    }

    SECTION("convenience modifier arithmetic with KeyInput") {
        REQUIRE(KeyInput('x', KeyInput::Modifier::SUPER) == ('x' + KeyInput::Modifier::SUPER));
    }

    SECTION("key conversion to and from glfw keys") {
        std::vector<int> glfw_keys = {GLFW_KEY_SPACE,
                                      GLFW_KEY_APOSTROPHE,
                                      GLFW_KEY_COMMA,
                                      GLFW_KEY_MINUS,
                                      GLFW_KEY_PERIOD,
                                      GLFW_KEY_SLASH,
                                      GLFW_KEY_0,
                                      GLFW_KEY_1,
                                      GLFW_KEY_2,
                                      GLFW_KEY_3,
                                      GLFW_KEY_4,
                                      GLFW_KEY_5,
                                      GLFW_KEY_6,
                                      GLFW_KEY_7,
                                      GLFW_KEY_8,
                                      GLFW_KEY_9,
                                      GLFW_KEY_SEMICOLON,
                                      GLFW_KEY_EQUAL,
                                      GLFW_KEY_A,
                                      GLFW_KEY_B,
                                      GLFW_KEY_C,
                                      GLFW_KEY_D,
                                      GLFW_KEY_E,
                                      GLFW_KEY_F,
                                      GLFW_KEY_G,
                                      GLFW_KEY_H,
                                      GLFW_KEY_I,
                                      GLFW_KEY_J,
                                      GLFW_KEY_K,
                                      GLFW_KEY_L,
                                      GLFW_KEY_M,
                                      GLFW_KEY_N,
                                      GLFW_KEY_O,
                                      GLFW_KEY_P,
                                      GLFW_KEY_Q,
                                      GLFW_KEY_R,
                                      GLFW_KEY_S,
                                      GLFW_KEY_T,
                                      GLFW_KEY_U,
                                      GLFW_KEY_V,
                                      GLFW_KEY_W,
                                      GLFW_KEY_X,
                                      GLFW_KEY_Y,
                                      GLFW_KEY_Z,
                                      GLFW_KEY_LEFT_BRACKET,
                                      GLFW_KEY_BACKSLASH,
                                      GLFW_KEY_RIGHT_BRACKET,
                                      GLFW_KEY_GRAVE_ACCENT,
                                      GLFW_KEY_WORLD_1,
                                      GLFW_KEY_WORLD_2,
                                      GLFW_KEY_ESCAPE,
                                      GLFW_KEY_ENTER,
                                      GLFW_KEY_TAB,
                                      GLFW_KEY_BACKSPACE,
                                      GLFW_KEY_INSERT,
                                      GLFW_KEY_DELETE,
                                      GLFW_KEY_RIGHT,
                                      GLFW_KEY_LEFT,
                                      GLFW_KEY_DOWN,
                                      GLFW_KEY_UP,
                                      GLFW_KEY_PAGE_UP,
                                      GLFW_KEY_PAGE_DOWN,
                                      GLFW_KEY_HOME,
                                      GLFW_KEY_END,
                                      GLFW_KEY_CAPS_LOCK,
                                      GLFW_KEY_SCROLL_LOCK,
                                      GLFW_KEY_NUM_LOCK,
                                      GLFW_KEY_PRINT_SCREEN,
                                      GLFW_KEY_PAUSE,
                                      GLFW_KEY_F1,
                                      GLFW_KEY_F2,
                                      GLFW_KEY_F3,
                                      GLFW_KEY_F4,
                                      GLFW_KEY_F5,
                                      GLFW_KEY_F6,
                                      GLFW_KEY_F7,
                                      GLFW_KEY_F8,
                                      GLFW_KEY_F9,
                                      GLFW_KEY_F10,
                                      GLFW_KEY_F11,
                                      GLFW_KEY_F12,
                                      GLFW_KEY_F13,
                                      GLFW_KEY_F14,
                                      GLFW_KEY_F15,
                                      GLFW_KEY_F16,
                                      GLFW_KEY_F17,
                                      GLFW_KEY_F18,
                                      GLFW_KEY_F19,
                                      GLFW_KEY_F20,
                                      GLFW_KEY_F21,
                                      GLFW_KEY_F22,
                                      GLFW_KEY_F23,
                                      GLFW_KEY_F24,
                                      GLFW_KEY_F25,
                                      GLFW_KEY_KP_0,
                                      GLFW_KEY_KP_1,
                                      GLFW_KEY_KP_2,
                                      GLFW_KEY_KP_3,
                                      GLFW_KEY_KP_4,
                                      GLFW_KEY_KP_5,
                                      GLFW_KEY_KP_6,
                                      GLFW_KEY_KP_7,
                                      GLFW_KEY_KP_8,
                                      GLFW_KEY_KP_9,
                                      GLFW_KEY_KP_DECIMAL,
                                      GLFW_KEY_KP_DIVIDE,
                                      GLFW_KEY_KP_MULTIPLY,
                                      GLFW_KEY_KP_SUBTRACT,
                                      GLFW_KEY_KP_ADD,
                                      GLFW_KEY_KP_ENTER,
                                      GLFW_KEY_KP_EQUAL,
                                      GLFW_KEY_LEFT_SHIFT,
                                      GLFW_KEY_LEFT_CONTROL,
                                      GLFW_KEY_LEFT_ALT,
                                      GLFW_KEY_LEFT_SUPER,
                                      GLFW_KEY_RIGHT_SHIFT,
                                      GLFW_KEY_RIGHT_CONTROL,
                                      GLFW_KEY_RIGHT_ALT,
                                      GLFW_KEY_RIGHT_SUPER,
                                      GLFW_KEY_MENU,
                                      GLFW_KEY_UNKNOWN};
        for (int glfw_key : glfw_keys) {
            const KeyInput notf_key(glfw_key);
            REQUIRE(notf_key.to_glfw_key() == glfw_key);
        }
        for (char character :
             "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ!1@2#3$4%5^6&7*8(9)0 \"'<,_->.?/:;+={[|\\}]`~`ö") {
            const KeyInput key(character); // for coverage only
        }
    }
}
