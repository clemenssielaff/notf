#pragma once

#include "app/forwards.hpp"
#include "app/io/event.hpp"
#include "common/size2.hpp"

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
    ~WindowEvent() override;

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// The type of this event.
    const Type type;
};

//====================================================================================================================//

/// Event object generated when a Window is resized.
/// Unlike other events, this one cannot be handled by a Layer in the front but is always propagated all the way to the
/// last Layer in a Window.
class WindowResizeEvent : public detail::EventBase<WindowResizeEvent> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param type         The type of this event.
    WindowResizeEvent(Size2i old_size, Size2i new_size) : old_size(std::move(old_size)), new_size(std::move(new_size))
    {}

    /// Destructor.
    ~WindowResizeEvent() override;

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// Old size of the Window.
    const Size2i old_size;

    /// New size of the Window.
    const Size2i new_size;
};

NOTF_CLOSE_NAMESPACE
