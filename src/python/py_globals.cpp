#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/input.hpp"
#include "python/docstr.hpp"
using namespace notf;

namespace { // anonymous

/**
 * Object acting as the namespace for global enums.
 */
struct NotfNamespace {
};

} // namespace  anonymous

void produce_globals(pybind11::module& module)
{
    py::class_<NotfNamespace> Py_NotfNamespace(module, "notf");

    py::enum_<Key>(Py_NotfNamespace, "Key")
        .value("APOSTROPHE", Key::APOSTROPHE)
        .value("COMMA", Key::COMMA)
        .value("MINUS", Key::MINUS)
        .value("PERIOD", Key::PERIOD)
        .value("SLASH", Key::SLASH)
        .value("ZERO", Key::ZERO)
        .value("ONE", Key::ONE)
        .value("TWO", Key::TWO)
        .value("THREE", Key::THREE)
        .value("FOUR", Key::FOUR)
        .value("FIVE", Key::FIVE)
        .value("SIX", Key::SIX)
        .value("SEVEN", Key::SEVEN)
        .value("EIGHT", Key::EIGHT)
        .value("NINE", Key::NINE)
        .value("SEMICOLON", Key::SEMICOLON)
        .value("EQUAL", Key::EQUAL)
        .value("A", Key::A)
        .value("B", Key::B)
        .value("C", Key::C)
        .value("D", Key::D)
        .value("E", Key::E)
        .value("F", Key::F)
        .value("G", Key::G)
        .value("H", Key::H)
        .value("I", Key::I)
        .value("J", Key::J)
        .value("K", Key::K)
        .value("L", Key::L)
        .value("M", Key::M)
        .value("N", Key::N)
        .value("O", Key::O)
        .value("P", Key::P)
        .value("Q", Key::Q)
        .value("R", Key::R)
        .value("S", Key::S)
        .value("T", Key::T)
        .value("U", Key::U)
        .value("V", Key::V)
        .value("W", Key::W)
        .value("X", Key::X)
        .value("Y", Key::Y)
        .value("Z", Key::Z)
        .value("LEFT_BRACKET", Key::LEFT_BRACKET)
        .value("BACKSLASH", Key::BACKSLASH)
        .value("RIGHT_BRACKET", Key::RIGHT_BRACKET)
        .value("GRAVE_ACCENT", Key::GRAVE_ACCENT)
        .value("WORLD_1", Key::WORLD_1)
        .value("WORLD_2", Key::WORLD_2)
        .value("ESCAPE", Key::ESCAPE)
        .value("ENTER", Key::ENTER)
        .value("TAB", Key::TAB)
        .value("BACKSPACE", Key::BACKSPACE)
        .value("INSERT", Key::INSERT)
        .value("DELETE", Key::DELETE)
        .value("RIGHT", Key::RIGHT)
        .value("LEFT", Key::LEFT)
        .value("DOWN", Key::DOWN)
        .value("UP", Key::UP)
        .value("PAGE_UP", Key::PAGE_UP)
        .value("PAGE_DOWN", Key::PAGE_DOWN)
        .value("HOME", Key::HOME)
        .value("END", Key::END)
        .value("CAPS_LOCK", Key::CAPS_LOCK)
        .value("SCROLL_LOCK", Key::SCROLL_LOCK)
        .value("NUM_LOCK", Key::NUM_LOCK)
        .value("PRINT_SCREEN", Key::PRINT_SCREEN)
        .value("PAUSE", Key::PAUSE)
        .value("F1", Key::F1)
        .value("F2", Key::F2)
        .value("F3", Key::F3)
        .value("F4", Key::F4)
        .value("F5", Key::F5)
        .value("F6", Key::F6)
        .value("F7", Key::F7)
        .value("F8", Key::F8)
        .value("F9", Key::F9)
        .value("F10", Key::F10)
        .value("F11", Key::F11)
        .value("F12", Key::F12)
        .value("F13", Key::F13)
        .value("F14", Key::F14)
        .value("F15", Key::F15)
        .value("F16", Key::F16)
        .value("F17", Key::F17)
        .value("F18", Key::F18)
        .value("F19", Key::F19)
        .value("F20", Key::F20)
        .value("F21", Key::F21)
        .value("F22", Key::F22)
        .value("F23", Key::F23)
        .value("F24", Key::F24)
        .value("F25", Key::F25)
        .value("KP_0", Key::KP_0)
        .value("KP_1", Key::KP_1)
        .value("KP_2", Key::KP_2)
        .value("KP_3", Key::KP_3)
        .value("KP_4", Key::KP_4)
        .value("KP_5", Key::KP_5)
        .value("KP_6", Key::KP_6)
        .value("KP_7", Key::KP_7)
        .value("KP_8", Key::KP_8)
        .value("KP_9", Key::KP_9)
        .value("KP_DECIMAL", Key::KP_DECIMAL)
        .value("KP_DIVIDE", Key::KP_DIVIDE)
        .value("KP_MULTIPLY", Key::KP_MULTIPLY)
        .value("KP_SUBTRACT", Key::KP_SUBTRACT)
        .value("KP_ADD", Key::KP_ADD)
        .value("KP_ENTER", Key::KP_ENTER)
        .value("KP_EQUAL", Key::KP_EQUAL)
        .value("LEFT_SHIFT", Key::LEFT_SHIFT)
        .value("LEFT_CONTROL", Key::LEFT_CONTROL)
        .value("LEFT_ALT", Key::LEFT_ALT)
        .value("LEFT_SUPER", Key::LEFT_SUPER)
        .value("RIGHT_SHIFT", Key::RIGHT_SHIFT)
        .value("RIGHT_CONTROL", Key::RIGHT_CONTROL)
        .value("RIGHT_ALT", Key::RIGHT_ALT)
        .value("RIGHT_SUPER", Key::RIGHT_SUPER)
        .value("MENU", Key::MENU)
        .value("INVALID", Key::INVALID);

    py::enum_<Button>(Py_NotfNamespace, "Button")
        .value("BUTTON_1", Button::BUTTON_1)
        .value("BUTTON_2", Button::BUTTON_2)
        .value("BUTTON_3", Button::BUTTON_3)
        .value("BUTTON_4", Button::BUTTON_4)
        .value("BUTTON_5", Button::BUTTON_5)
        .value("BUTTON_6", Button::BUTTON_6)
        .value("BUTTON_7", Button::BUTTON_7)
        .value("BUTTON_8", Button::BUTTON_8)
        .value("NONE", Button::NONE)
        .value("LEFT", Button::LEFT)
        .value("RIGHT", Button::RIGHT)
        .value("MIDDLE", Button::MIDDLE)
        .value("INVALID", Button::INVALID);

    py::enum_<KeyAction>(Py_NotfNamespace, "KeyAction")
        .value("RELEASE", KeyAction::RELEASE)
        .value("PRESS", KeyAction::PRESS)
        .value("REPEAT", KeyAction::REPEAT);

    py::enum_<MouseAction>(Py_NotfNamespace, "MouseAction")
        .value("RELEASE", MouseAction::RELEASE)
        .value("MOVE", MouseAction::MOVE)
        .value("PRESS", MouseAction::PRESS);

    py::enum_<KeyModifiers>(Py_NotfNamespace, "KeyModifiers")
        .value("NONE", KeyModifiers::NONE)
        .value("SHIFT", KeyModifiers::SHIFT)
        .value("CTRL", KeyModifiers::CTRL)
        .value("ALT", KeyModifiers::ALT)
        .value("SUPER", KeyModifiers::SUPER);
}
