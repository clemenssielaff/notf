#pragma once

#include "app/forwards.hpp"
#include "common/vector2.hpp"
#include "event.hpp"
#include "keyboard.hpp"

NOTF_OPEN_NAMESPACE

/// Event object generated when the Application notices a mouse input.
class MouseEvent : public detail::EventBase<MouseEvent> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window_pos   Position of the mouse in the Window.
    /// @param window_delta How much the mouse moved since the last event.
    /// @param button       Contains the button that triggered the event, if one exists.
    /// @param action       Action that triggered this event.
    MouseEvent(Vector2f window_pos, Vector2f window_delta, Button button, MouseAction action, KeyModifiers modifiers,
               const ButtonStateSet& stateset)
        : window_pos(std::move(window_pos))
        , window_delta(std::move(window_delta))
        , button(button)
        , action(action)
        , modifiers(modifiers)
        , stateset(stateset)
        , m_was_handled(false)
    {}

    /// Destructor.
    virtual ~MouseEvent() override;

    /// Checks whether this event was already handled or not.
    bool was_handled() const { return m_was_handled; }

    /// Must be called after an event handler handled this event.
    void set_handled() { m_was_handled = true; }

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// Position of the mouse cursor relative to the top-left corner of `window`.
    const Vector2f window_pos;

    /// Delta of the mouse cursor since the last event, in window coordinates.
    /// If this is a "scroll" event, this field holds the scroll delta instead.
    const Vector2f window_delta;

    /// The mouse button that triggered this event.
    /// Is set to Button::INVALID when this is a "move" or "scroll" event.
    const Button button;

    /// The action that triggered this event.
    const MouseAction action;

    /// Mask of all active keyboard modifiers for this event.
    const KeyModifiers modifiers;

    /// The state of all mouse buttons.
    const ButtonStateSet& stateset;

private:
    /// True iff this event was already handled.
    bool m_was_handled;
};

NOTF_CLOSE_NAMESPACE
