#pragma once

#include "notf/common/fibers.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/event/event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

class Scheduler {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param buffer_size  Number of items in the Event handling buffer before `schedule` blocks.
    Scheduler(size_t buffer_size = 128);

    /// Destructor.
    ~Scheduler() { m_event_queue.close(); }

    /// Schedules a new event to be handled on the event thread.
    void schedule(AnyEvent&& event) { m_event_queue.push(std::move(event)); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Event handling thread.
    Thread m_event_handler;

    /// MPMC queue buffering Events for the event handling thread.
    fibers::buffered_channel<AnyEvent> m_event_queue;
};

NOTF_CLOSE_NAMESPACE
