#pragma once

#include "app/io/event.hpp"
#include "app/scene.hpp"

NOTF_OPEN_NAMESPACE

/// Event object scheduled for a SceneGraph to change its Composition.
struct CompositionChangeEvent : public detail::EventBase<CompositionChangeEvent> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param window           Window containing the SceneGraph that the event is meant for.
    /// @param new_composition  New Composition of the SceneGraph after the event was handled.
    CompositionChangeEvent(valid_ptr<const Window*> window, valid_ptr<SceneGraph::CompositionPtr> new_composition)
        : detail::EventBase<CompositionChangeEvent>(window), new_composition(std::move(new_composition))
    {}

    /// Destructor.
    ~CompositionChangeEvent() override;

    // fields ------------------------------------------------------------------------------------------------------- //
public:
    /// New Composition of the SceneGraph after the event was handled.
    const valid_ptr<SceneGraph::CompositionPtr> new_composition;
};

NOTF_CLOSE_NAMESPACE
