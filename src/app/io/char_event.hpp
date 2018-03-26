#pragma once

#include "app/forwards.hpp"
#include "common/utf.hpp"
#include "event.hpp"
#include "keyboard.hpp"

NOTF_OPEN_NAMESPACE

/// Event object generated when the Application receives a keyboard input that represents a unicode codepoint.
class CharEvent : public detail::EventBase<CharEvent> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window       Window that received the event from the Application.
    /// @param codepoint    Unicode codepoint.
    /// @param modifiers    Additional modifiers that were held when the event was generated.
    /// @param stateset     State of all keys on the keyboard at the time the event was generated.
    CharEvent(Window& window, utf32_t codepoint, KeyModifiers modifiers, const KeyStateSet& stateset)
        : window(window), codepoint(codepoint), modifiers(modifiers), stateset(stateset), m_was_handled(false)
    {}

    /// Destructor.
    virtual ~CharEvent() override;

    /// Checks whether this event was already handled or not.
    bool was_handled() const { return m_was_handled; }

    /// Must be called after an event handler handled this event.
    void set_handled() { m_was_handled = true; }

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// The Window to which the event was sent.
    const Window& window;

    /// The input character codepoint as native endian UTF-32.
    const Codepoint codepoint;

    /// Mask of all active keyboard modifiers for this event.
    const KeyModifiers modifiers;

    /// The state of all keys.
    const KeyStateSet& stateset;

private:
    /// True iff this event was already handled.
    bool m_was_handled;
};

NOTF_CLOSE_NAMESPACE
