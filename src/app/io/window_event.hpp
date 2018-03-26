#pragma once

#include "app/forwards.hpp"
#include "event.hpp"

NOTF_OPEN_NAMESPACE

/// Event object generated when the Application receives a keyboard input that represents a unicode codepoint.
class WindowEvent : public detail::EventBase<WindowEvent> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    enum class Type {
        CURSOR_ENTERED, ///< Emitted when the mouse cursor entered the client area of this Window.
        CURSOR_EXITED,  ///< Emitted when the mouse cursor exited the client area of this Window.
        CLOSE,          ///< Emitted just before this Window is closed.
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window       Window that received the event from the Application.
    /// @param type         The type of this event.
    WindowEvent(Window& window, Type type) : window(window), type(type) {}

    /// Destructor.
    virtual ~WindowEvent() override;

    /// Checks whether this event was already handled or not.
    bool was_handled() const { return m_was_handled; }

    /// Must be called after an event handler handled this event.
    void set_handled() { m_was_handled = true; }

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// The Window to which the event was sent.
    const Window& window;

    /// The type of this event.
    const Type type;

private:
    /// True iff this event was already handled.
    bool m_was_handled = false;
};

NOTF_CLOSE_NAMESPACE
