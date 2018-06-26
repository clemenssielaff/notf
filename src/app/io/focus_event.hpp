#pragma once

#include "app/io/event.hpp"
#include "app/io/keyboard.hpp"
#include "app/node_handle.hpp"
#include "app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

/// Event object generated when a Node gains or looses focus.
/// Unlike other events, a 'focus gained' event is propagated up the hierarchy if (and only if) the receiving Widget
/// handles it.
/// 'focus lost' events are handled by design.
/// Layouts will never get to see an unhandled FocusEvent.
struct FocusEvent : public detail::EventBase<FocusEvent> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window       Window that the event is meant for.
    /// @param action       Action that triggered this event.
    /// @param old_focus    Widget that lost focus.
    /// @param new_focus    Widget that gained focus.
    FocusEvent(valid_ptr<const Window*> window, FocusAction action, NodeHandle<Widget> old_focus,
               NodeHandle<Widget> new_focus)
        : detail::EventBase<FocusEvent>(window)
        , action(action)
        , old_focus(std::move(old_focus))
        , new_focus(std::move(new_focus))
        , m_was_handled(action == FocusAction::LOST)
    {}

    /// Destructor.
    virtual ~FocusEvent() override;

    /// Checks whether this event was already handled or not.
    bool was_handled() const { return m_was_handled; }

    /// Must be called after an event handler handled this event.
    void set_handled() { m_was_handled = true; }

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// The action that triggered this event.
    const FocusAction action;

    /// Widget that lost the focus - may be empty.
    NodeHandle<Widget> old_focus;

    /// Widget that gained the focus - may be empty.
    NodeHandle<Widget> new_focus;

private:
    /// True iff this event was already handled.
    bool m_was_handled;
};

NOTF_CLOSE_NAMESPACE
