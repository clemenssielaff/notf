#pragma once

#include "app/forwards.hpp"
#include "app/io/keyboard.hpp"

namespace notf {

/// Event object generated when the Application receives a keyboard input.
class KeyEvent {
    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window       Window that received the event from the Application.
    /// @param key          Key that did something.
    /// @param action       What the key did.
    /// @param modifiers    Additional modifiers that were held when the event was generated.
    /// @param stateset     State of all keys on the keyboard at the time the event was generated.
    KeyEvent(Window& window, Key key, KeyAction action, KeyModifiers modifiers, const KeyStateSet& stateset)
        : window(window), key(key), action(action), modifiers(modifiers), stateset(stateset), m_was_handled(false)
    {}

    /// Checks whether this event was already handled or not.
    bool was_handled() const { return m_was_handled; }

    /// Must be called after an event handler handled this event.
    void set_handled() { m_was_handled = true; }

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// The Window to which the event was sent.
    const Window& window;

    /// The key that triggered this event.
    const Key key;

    /// The action that triggered this event.
    const KeyAction action;

    /// Mask of all active keyboard modifiers for this event.
    const KeyModifiers modifiers;

    /// The state of all keys.
    const KeyStateSet& stateset;

private:
    /// True iff this event was already handled.
    bool m_was_handled;
};

} // namespace notf
