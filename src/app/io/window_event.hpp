#pragma once

#include "app/forwards.hpp"
#include "event.hpp"

NOTF_OPEN_NAMESPACE

/// Event object generated when the Application receives a Window event.
/// Unlike other events, this one cannot be handled by a Layer in the front but is always propagated all the way to the
/// last Layer in a Window.
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
    /// @param type         The type of this event.
    WindowEvent(Type type) : type(type) {}

    /// Destructor.
    virtual ~WindowEvent() override;

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// The type of this event.
    const Type type;
};

NOTF_CLOSE_NAMESPACE
