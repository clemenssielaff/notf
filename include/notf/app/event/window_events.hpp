#pragma once

#include "notf/app/window.hpp"

NOTF_OPEN_NAMESPACE

// window events ==================================================================================================== //

/// Generated when the mouse cursor entered the client area of a Window.
struct WindowCursorEnteredEvent {
    const WindowHandle window;
};

/// Generated when the mouse cursor left the client area of a Window.
struct WindowCursorExitedEvent {
    const WindowHandle window;
};

/// Generated when the OS requests a Window to close.
struct WindowCloseEvent {
    const WindowHandle window;
};

/// Generated when a Window is moved.
struct WindowMoveEvent {
    const WindowHandle window;
    const V2i position;
};

/// Generated when a Window is resized.
struct WindowResizeEvent {
    const WindowHandle window;
    const Size2i size;
};

/// Generated when the framebuffer of a Window is resized.
struct WindowResolutionChangeEvent {
    const WindowHandle window;
    const Size2i resolution;
};

/// Generated when a Window has gained OS focus.
struct WindowFocusGainEvent {
    const WindowHandle window;
};

/// Generated when a Window has lost OS focus.
struct WindowFocusLostEvent {
    const WindowHandle window;
};

/// Generated when a Window is refreshed from the OS.
struct WindowRefreshEvent {
    const WindowHandle window;
};

/// Generated when a Window is minimized.
struct WindowMinimizeEvent {
    const WindowHandle window;
};

/// Generated when a Window is restored (opposite of minimized).
struct WindowRestoredEvent {
    const WindowHandle window;
};

/// Generated when when the user drops one or more files into the Window.
struct WindowFileDropEvent {
    const WindowHandle window;
    const uint count;
    const char** const paths;
};

NOTF_CLOSE_NAMESPACE
