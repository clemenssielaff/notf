#pragma once

#include <core/keyboard.hpp>

namespace signal {

/// \brief A key event.
///
struct KeyEvent {

    /// \brief The Window to which the event was sent.
    const Window* window;

    /// \brief The key that triggered this event.
    const KEY key;

    /// \brief The action that triggered this event.
    const KEY_ACTION action;

    /// \brief Mask of all active modifiers for this event.
    const KEY_MODIFIERS modifiers;

    /// \brief The state of all keys.
    const KeyStateSet stateset;
};

} // namespace signal
