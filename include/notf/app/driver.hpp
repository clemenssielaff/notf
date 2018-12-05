#pragma once

#include <set>

#include "notf/app/input.hpp"
#include "notf/app/window.hpp"

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
    /// Default (empty) Constructor.
    Driver() = default;

    /// Value constructor.
    /// @param window   Window to attach to.
    Driver(WindowHandle window) : m_window(std::move(window)) {}

    // window attachement -----------------------------------------------------

    /// Attaches to a given Window.
    /// @param window   Window to attach to.
    void attach_to(WindowHandle window) { m_window = std::move(window); }

    /// Detaches from the attached Window or does nothing.
    void detach() { m_window = {}; }

    /// Tests if this Driver is attached to a Window or not.
    bool is_attached() const { return m_window != WindowHandle{}; }

    /// The Window this Driver is attached to, is invalid if it is not attached.
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
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void key_stroke(Key::Token key);

    /// Simulate a key press.
    /// @param key          Pressed Key.
    /// @throws InputError  If the key is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void key_press(Key::Token key);

    /// Simulate a key repeat (hold).
    /// @param key          Held key.
    /// @throws InputError  If the key is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void key_hold(Key::Token key);

    /// Simulate a key release.
    /// @param key          Released key.
    /// @throws InputError  If the key is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void key_release(Key::Token key);

    /// Moves the mouse cursor to a given position inside the Window.
    /// @param pos          Position of the cursor in Window coordinates.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void mouse_move(V2d pos);

    /// Simulate a mouse button click.
    /// @param button       Clicked button.
    /// @throws InputError  If the button is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void mouse_click(Mouse::Button button);

    /// Simulate a mouse button press.
    /// @param button       Pressed button.
    /// @throws InputError  If the button is already registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void mouse_press(Mouse::Button button);

    /// Simulate a mouse button release.
    /// @param button       Released button.
    /// @throws InputError  If the button is not registered as being pressed.
    /// @throws HandleExpiredError  If the Driver is not attached to a Window.
    void mouse_release(Mouse::Button button);

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Window that this driver is currently attached to.
    WindowHandle m_window;

    /// Active key modifier.
    Key::Modifier m_modifier;

    /// Last recorded mouse position.
    V2d m_mouse_position = {-1, -1};

    /// All mouse buttons currently being pressed.
    std::set<Mouse::Button> m_pressed_buttons;

    /// All keys currently being pressed.
    std::set<Key::Token> m_pressed_keys;
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
    Mouse(::notf::Mouse::Button button, const int x = -1, const int y = -1) : m_button(button), m_pos{x, y} {}
    void run(Driver& driver) final;

private:
    ::notf::Mouse::Button m_button;
    V2d m_pos;
};

// convenience variables in driver namespace ======================================================================== //

constexpr auto CTRL = ::notf::Key::Modifier::CTRL;
constexpr auto SHIFT = ::notf::Key::Modifier::SHIFT;
constexpr auto ALT = ::notf::Key::Modifier::ALT;
constexpr auto SUPER = ::notf::Key::Modifier::SUPER;

constexpr auto LEFT = ::notf::Mouse::Button::LEFT;
constexpr auto RIGHT = ::notf::Mouse::Button::RIGHT;
constexpr auto MIDDLE = ::notf::Mouse::Button::MIDDLE;

} // namespace driver

NOTF_CLOSE_NAMESPACE
