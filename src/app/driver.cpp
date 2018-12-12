#include "notf/app/driver.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/application.hpp"
#include "notf/app/glfw_callbacks.hpp"

#include "notf/graphic/glfw.hpp"

NOTF_OPEN_NAMESPACE

// driver =========================================================================================================== //

Driver::Driver(WindowHandle window) : m_window(std::move(window)) {
    if (m_window.is_expired()) { NOTF_THROW(HandleExpiredError, "Cannot create Driver with an expired Window Handle"); }
}

Driver& Driver::operator<<(driver::detail::AnyInput&& input) {
    input.run(*this);
    return *this;
}

Driver& Driver::operator<<(const char character) {
    GLFWwindow* window = m_window.get_glfw_window();
    const KeyInput key(character);
    const KeyInput::Modifier modifier = key.modifier + m_modifier;
    TheApplication()->schedule([window, key, character, modifier] {
        GlfwCallbacks::_on_token_key(window, key.to_glfw_key(), key.scancode, GLFW_PRESS, to_number(modifier));
        GlfwCallbacks::_on_char_input(window, static_cast<uint>(character));
        GlfwCallbacks::_on_token_key(window, key.to_glfw_key(), key.scancode, GLFW_RELEASE, to_number(modifier));
    });
    return *this;
}

void Driver::key_stroke(KeyInput::Token key) {
    key_press(key);
    key_release(key);
}

void Driver::key_press(KeyInput::Token key) {
    if (m_pressed_keys.count(key) != 0) { NOTF_THROW(InputError, "Key {} is already pressed", to_number(key)); }
    m_pressed_keys.insert(key);
    TheApplication()->schedule([window = m_window.get_glfw_window(), key, modifier = m_modifier] {
        GlfwCallbacks::_on_token_key(window, to_number(key), 0, GLFW_PRESS, to_number(modifier));
    });
}

void Driver::key_hold(KeyInput::Token key) {
    if (m_pressed_keys.count(key) == 0) {
        NOTF_THROW(InputError, "Cannot hold key {} as it is not pressed", to_number(key));
    }
    TheApplication()->schedule([window = m_window.get_glfw_window(), key, modifier = m_modifier] {
        GlfwCallbacks::_on_token_key(window, to_number(key), 0, GLFW_REPEAT, to_number(modifier));
    });
}

void Driver::key_release(KeyInput::Token key) {
    if (m_pressed_keys.count(key) == 0) {
        NOTF_THROW(InputError, "Cannot release key {} as it is not pressed", to_number(key));
    }
    m_pressed_keys.erase(key);
    TheApplication()->schedule([window = m_window.get_glfw_window(), key, modifier = m_modifier] {
        GlfwCallbacks::_on_token_key(window, to_number(key), 0, GLFW_RELEASE, to_number(modifier));
    });
}

void Driver::mouse_move(const V2d pos) {
    m_mouse_position = pos;
    GlfwCallbacks::_on_cursor_move(m_window.get_glfw_window(), pos.x(), pos.y());
}

void Driver::mouse_click(MouseInput::Button button) {
    mouse_press(button);
    mouse_release(button);
}

void Driver::mouse_press(const MouseInput::Button button) {
    if (m_pressed_buttons.count(button) != 0) {
        NOTF_THROW(InputError, "Mouse button {} is already pressed", to_number(button));
    }
    m_pressed_buttons.insert(button);
    TheApplication()->schedule([window = m_window.get_glfw_window(), button, modifier = m_modifier] {
        GlfwCallbacks::_on_mouse_button(window, to_number(button), GLFW_PRESS, to_number(modifier));
    });
}

void Driver::mouse_release(const MouseInput::Button button) {
    if (m_pressed_buttons.count(button) == 0) {
        NOTF_THROW(InputError, "Cannot release mouse button {} as it it not pressed", to_number(button));
    }
    m_pressed_buttons.erase(button);
    TheApplication()->schedule([window = m_window.get_glfw_window(), button, modifier = m_modifier] {
        GlfwCallbacks::_on_mouse_button(window, to_number(button), GLFW_RELEASE, to_number(modifier));
    });
}

// driver inputs ==================================================================================================== //

namespace driver {

void Mouse::run(Driver& driver) {
    if (m_pos != V2d{-1, -1}) { driver.mouse_move(m_pos); }
    driver.mouse_click(m_button);
}

} // namespace driver

NOTF_CLOSE_NAMESPACE
