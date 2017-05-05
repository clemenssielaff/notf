#pragma once

#include "common/input.hpp"

namespace notf {

class Window;

/** Event object generated when the Application receives a keyboard input. */
class KeyEvent {
public: // methods
    KeyEvent(Window& window, Key key, KeyAction action, KeyModifiers modifiers, const KeyStateSet& stateset)
        : window(window)
        , key(key)
        , action(action)
        , modifiers(modifiers)
        , stateset(stateset)
        , m_was_handled(false)
    {
    }

    /** Checks whether this event was already handled or not. */
    bool was_handled() const { return m_was_handled; }

    /** Must be called after an event handler handled this event. */
    void set_handled() { m_was_handled = true; }

public: // fields
    /** The Window to which the event was sent. */
    const Window& window;

    /** The key that triggered this event. */
    const Key key;

    /** The action that triggered this event. */
    const KeyAction action;

    /** Mask of all active keyboard modifiers for this event. */
    const KeyModifiers modifiers;

    /** The state of all keys. */
    const KeyStateSet& stateset;

private: // fields
    /** True iff this event was already handled. */
    bool m_was_handled;
};

} // namespace notf
