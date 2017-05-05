#pragma once

#include <memory>

#include "common/input.hpp"

namespace notf {

class Window;
class Widget;

using WidgetPtr = std::shared_ptr<Widget>;

/** Event object generated when an Item gains or looses focus.
 * Unlike other events, a 'focus gained' event is propagated up the hierarchy if (and only if) the receiving Widget
 * handles it.
 * 'focus lost' events are handled by design.
 * Layouts will never get to see an unhandled FocusEvent.
 */
class FocusEvent {
public: // methods
    FocusEvent(Window& window, FocusAction action, WidgetPtr old_focus, WidgetPtr new_focus)
        : window(window)
        , action(action)
        , old_focus(std::move(old_focus))
        , new_focus(std::move(new_focus))
        , m_was_handled(action == FocusAction::LOST)
    {
    }

    /** Checks whether this event was already handled or not. */
    bool was_handled() const { return m_was_handled; }

    /** Must be called after an event handler handled this event. */
    void set_handled() { m_was_handled = true; }

public: // fields
    /** The Window to which the event was sent. */
    const Window& window;

    /** The action that triggered this event. */
    const FocusAction action;

    /** Widget that lost the focus - may be empty. */
    WidgetPtr const old_focus;

    /** Widget that gained the focus - may be empty. */
    WidgetPtr const new_focus;

private: // fields
    /** True iff this event was already handled. */
    bool m_was_handled;
};

} // namespace notf
