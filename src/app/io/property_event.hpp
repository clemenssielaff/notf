#pragma once

#include "app/io/event.hpp"
#include "app/property_graph.hpp"

NOTF_OPEN_NAMESPACE

/// Event generated when the value of an associated Property was changed.
struct PropertyEvent : public detail::EventBase<PropertyEvent> {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Constructor.
    /// @param window   Window that the event is meant for.
    /// @param updates  Updates contained in this Event.
    PropertyEvent(valid_ptr<const Window*> window, PropertyUpdateList&& updates)
        : detail::EventBase<PropertyEvent>(window), updates(std::move(updates))
    {}

    /// Destructor.
    ~PropertyEvent() override;

    // fields --------------------------------------------------------------------------------------------------------//
public:
    /// Updates contained in this Event.
    PropertyUpdateList updates;
};

NOTF_CLOSE_NAMESPACE
