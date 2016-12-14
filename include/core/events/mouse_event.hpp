#pragma once

#include "common/input.hpp"
#include "common/vector2.hpp"

namespace notf {

class Window;

/** Event object generated when the Application notices a mouse input. */
class MouseEvent {

    friend class Window;

public: // methods
    MouseEvent(Window* window, Vector2 window_pos, Button button,
               MouseAction action, KeyModifiers modifiers, const ButtonStateSet& stateset)
        : window(window)
        , window_pos(std::move(window_pos))
        , window_delta(Vector2::zero()) // TODO: some means to set the MouseEvent delta on a per-window basis
        , button(button)
        , action(action)
        , modifiers(modifiers)
        , stateset(stateset)
        , m_was_handled(false)
    {
    }

    /** Checks whether this event was already handled or not. */
    bool was_handled() const { return m_was_handled; }

    /** Called when some event handler handled this event. */
    void set_handled() { m_was_handled = true; }

public: // fields
    /** The Window to which the event was sent. */
    const Window* window;

    /** Position of the mouse cursor relative to the top-left corner of `window`. */
    const Vector2 window_pos;

    /** Delta of the mouse cursor since the last event, in window coordinates. */
    const Vector2 window_delta;

    /** The mouse button that triggered this event, is set to Button::INVALID when this is a "move" event. */
    const Button button;

    /** The action that triggered this event. */
    const MouseAction action;

    /** Mask of all active keyboard modifiers for this event. */
    const KeyModifiers modifiers;

    /** The state of all mouse buttons. */
    const ButtonStateSet& stateset;

private: // fields
    /** True iff this event was already handled. */
    bool m_was_handled = false;
};

} // namespace notf
