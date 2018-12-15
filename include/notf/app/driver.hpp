#pragma once

#include <set>

#include "notf/app/graph/window.hpp"
#include "notf/app/input.hpp"

NOTF_OPEN_NAMESPACE

// driver =========================================================================================================== //

/// The Application "Driver" is the point at which all events from the outside are dispatched into the Application
/// (usually the EventHandler) to modify the Graph.
/// Additionally, the user can create instances of the ApplicationDriver class and generate simluated events
/// programatically, that are indistinguishable to real ones. This is helpful for low-level macro recording, testing
/// and whatnot.
class Driver {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param window   Window to attach to.
    /// @throws HandleExpiredError  If the Window Handle is expired.
    Driver(WindowHandle window);

    /// The Window this Driver is attached to.
    WindowHandle get_window() const { return m_window; }

    // event handling ---------------------------------------------------------

    /// Ingest a single Input object.
    Driver& operator<<(driver::detail::AnyInput&& input);

    /// Simulates a single ascii character key stroke.
    Driver& operator<<(const char character);

    /// Simulates a sequence of character key strokes.
    Driver& operator<<(const char* string) {
        while (*string != '\0') {
            operator<<(*string++);
        }
        return *this;
    }

    // ui state ---------------------------------------------------------------

    /// Simulate a key stroke.
    /// @param key          Key being hit and released.
    /// @throws InputError  If the key is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void key_stroke(KeyInput::Token key);

    /// Simulate a key press.
    /// @param key          Pressed Key.
    /// @throws InputError  If the key is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void key_press(KeyInput::Token key);

    /// Simulate a key repeat (hold).
    /// @param key          Held key.
    /// @throws InputError  If the key is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void key_hold(KeyInput::Token key);

    /// Simulate a key release.
    /// @param key          Released key.
    /// @throws InputError  If the key is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void key_release(KeyInput::Token key);

    /// Moves the mouse cursor to a given position inside the Window.
    /// @param pos          Position of the cursor in Window coordinates.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void mouse_move(V2d pos);

    /// Simulate a mouse button click.
    /// @param button       Clicked button.
    /// @throws InputError  If the button is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void mouse_click(MouseInput::Button button);

    /// Simulate a mouse button press.
    /// @param button       Pressed button.
    /// @throws InputError  If the button is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void mouse_press(MouseInput::Button button);

    /// Simulate a mouse button release.
    /// @param button       Released button.
    /// @throws InputError  If the button is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver's Window Handle has expired.
    void mouse_release(MouseInput::Button button);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Window that this driver is attached to.
    WindowHandle m_window;

    /// Active key modifier.
    KeyInput::Modifier m_modifier;

    /// Last recorded mouse position.
    V2d m_mouse_position = {-1, -1};

    /// All mouse buttons currently being pressed.
    std::set<MouseInput::Button> m_pressed_buttons;

    /// All keys currently being pressed.
    std::set<KeyInput::Token> m_pressed_keys;
};

// driver inputs ==================================================================================================== //

/// The "driver" namespace contains functions and variables that allow convenient "driving" of an Application using
/// programmable input.
///
///     using namespace driver; // use various driver-specific types
///
///     Driver driver(window); // a Driver attaches to a single Window
///
///     driver << Mouse(LEFT, 20, 20);            // click left at window position
///     driver << 'K';                            // char event uppercase k
///     driver << Key('K', CTRL);                 // key event CTRL+ k + SHIFT (implicit)
///     driver << Mouse(RIGHT, button);           // click left at center of Widget "button"
///     driver << Mouse(MIDDLE, button, 10, 40);  // click left at widget position 10,40
///     driver << "Test";                         // 4 char events in sequence
///     driver << Mouse(LEFT)                     // ...
///            << Mouse(MIDDLE)                   // ...
///            << Mouse(RIGHT);                   // sequence of 3 mouse clicks at window center
///     driver << WindowResize(400, 200);         // window resize
namespace driver {

namespace detail {

struct AnyInput {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Virtual destructor.
    virtual ~AnyInput() = default;

    /// Executes the input function.
    /// @param driver   Driver handling this Input.
    virtual void run(Driver& driver) = 0;
};

} // namespace detail

struct Mouse : public detail::AnyInput {
    Mouse(::notf::MouseInput::Button button, const int x = -1, const int y = -1) : m_button(button), m_pos{x, y} {}
    void run(Driver& driver) final;

private:
    ::notf::MouseInput::Button m_button;
    V2d m_pos;
};

// convenience variables in driver namespace ======================================================================== //

constexpr auto CTRL = ::notf::KeyInput::Modifier::CTRL;
constexpr auto SHIFT = ::notf::KeyInput::Modifier::SHIFT;
constexpr auto ALT = ::notf::KeyInput::Modifier::ALT;
constexpr auto SUPER = ::notf::KeyInput::Modifier::SUPER;

constexpr auto LEFT = ::notf::MouseInput::Button::LEFT;
constexpr auto RIGHT = ::notf::MouseInput::Button::RIGHT;
constexpr auto MIDDLE = ::notf::MouseInput::Button::MIDDLE;

} // namespace driver

NOTF_CLOSE_NAMESPACE
