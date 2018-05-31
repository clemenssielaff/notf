#pragma once

#include "app/io/event.hpp"
#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

/// Event object scheduled for a Window to change its State.
struct StateChangeEvent : public detail::EventBase<StateChangeEvent> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window       Window that the event is meant for.
    /// @param new_state    New State of the Window after the event was handled.
    StateChangeEvent(valid_ptr<const Window*> window, valid_ptr<SceneGraph::StatePtr> new_state)
        : detail::EventBase<StateChangeEvent>(window), new_state(std::move(new_state))
    {}

    /// Destructor.
    ~StateChangeEvent() override;

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// New State of the Window after the event was handled.
    const valid_ptr<SceneGraph::StatePtr> new_state;
};

NOTF_CLOSE_NAMESPACE
