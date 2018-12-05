#include "notf/app/driver.hpp"

#include "notf/meta/log.hpp"

#include "notf/app/glfw.hpp"
#include "notf/app/glfw_callbacks.hpp"

NOTF_OPEN_NAMESPACE

// driver =========================================================================================================== //

Driver& Driver::operator<<(driver::detail::AnyInput&& input) {
    input.run(*this);
    return *this;
}

Driver& Driver::operator<<(const char character) {
    if (!is_attached()) {
        NOTF_LOG_WARN("Ignoring simulated keystroke \"{}\", because the ApplicationDriver is not attached to a Window",
                      character);
        return *this;
    }
    GLFWwindow* window = m_window.get_glfw_window();
    const Key key(character);
    const Key::Modifier modifier = key.modifier + m_modifier;
    GlfwCallbacks::_on_token_key(window, key.to_glfw_key(), key.scancode, GLFW_PRESS, to_number(modifier));
    GlfwCallbacks::_on_char_input(window, static_cast<uint>(character));
    GlfwCallbacks::_on_token_key(window, key.to_glfw_key(), key.scancode, GLFW_RELEASE, to_number(modifier));
    return *this;
}

void Driver::key_stroke(Key::Token key) {
    key_press(key);
    key_release(key);
}

void Driver::key_press(Key::Token key) {
    if (m_pressed_keys.count(key) != 0) { NOTF_THROW(InputError, "Key {} is already pressed", to_number(key)); }
    m_pressed_keys.insert(key);
    GlfwCallbacks::_on_token_key(m_window.get_glfw_window(), to_number(key), 0, GLFW_PRESS, to_number(m_modifier));
}

void Driver::key_hold(Key::Token key) {
    if (m_pressed_keys.count(key) == 0) {
        NOTF_THROW(InputError, "Cannot hold key {} as it is not pressed", to_number(key));
    }
    GlfwCallbacks::_on_token_key(m_window.get_glfw_window(), to_number(key), 0, GLFW_REPEAT, to_number(m_modifier));
}

void Driver::key_release(Key::Token key) {
    if (m_pressed_keys.count(key) == 0) {
        NOTF_THROW(InputError, "Cannot release key {} as it is not pressed", to_number(key));
    }
    m_pressed_keys.erase(key);
    GlfwCallbacks::_on_token_key(m_window.get_glfw_window(), to_number(key), 0, GLFW_RELEASE, to_number(m_modifier));
}

void Driver::mouse_move(const V2d pos) {
    m_mouse_position = pos;
    GlfwCallbacks::_on_cursor_move(m_window.get_glfw_window(), pos.x(), pos.y());
}

void Driver::mouse_click(Mouse::Button button) {
    mouse_press(button);
    mouse_release(button);
}

void Driver::mouse_press(const Mouse::Button button) {
    if (m_pressed_buttons.count(button) != 0) {
        NOTF_THROW(InputError, "Mouse button {} is already pressed", to_number(button));
    }
    m_pressed_buttons.insert(button);
    GlfwCallbacks::_on_mouse_button(m_window.get_glfw_window(), to_number(button), GLFW_PRESS, to_number(m_modifier));
}

void Driver::mouse_release(const Mouse::Button button) {
    if (m_pressed_buttons.count(button) == 0) {
        NOTF_THROW(InputError, "Cannot release mouse button {} as it it not pressed", to_number(button));
    }
    m_pressed_buttons.erase(button);
    GlfwCallbacks::_on_mouse_button(m_window.get_glfw_window(), to_number(button), GLFW_RELEASE, to_number(m_modifier));
}

// driver inputs ==================================================================================================== //

namespace driver {

void Mouse::run(Driver& driver) {
    if (!driver.is_attached()) { return; }
    if (m_pos != V2d{-1, -1}) { driver.mouse_move(m_pos); }
    driver.mouse_click(m_button);
    //
}

} // namespace driver

NOTF_CLOSE_NAMESPACE
