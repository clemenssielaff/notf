#pragma once

#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// window resize event ============================================================================================== //

/// Event object generated when a Window is resized.
/// Unlike other events, this one cannot be handled by a Layer in the front but is always propagated all the way to the
/// last Layer in a Window.
struct WindowResizeEvent {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window   Window that the event is meant for.
    /// @param new_size New size of the Window.
    WindowResizeEvent(WindowHandle window, Size2i new_size)
        : window(std::move(window)), new_size(std::move(new_size)) {}

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// Window that was resized.
    const WindowHandle window;

    /// New size of the Window.
    const Size2i new_size;
};

NOTF_CLOSE_NAMESPACE
