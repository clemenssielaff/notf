#pragma once

#include <deque>

#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/event/event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

class Scheduler {

    // types ----------------------------------------------------------------------------------- //
private:
    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    Scheduler();

    /// Destructor.
    ~Scheduler();

    /// Schedules a new event to be handled on the event thread.
    void schedule(AnyEvent&& event);

    /// Stop the event handler.
    /// Blocks until the thread has joined.
    void stop();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Event handling thread.
    Thread m_thread;

    /// All events for this handler in order of occurrence.
    /// (Events in the front are older than the ones in the back).
    std::deque<AnyEvent> m_events;

    /// Mutex guarding the RenderThread's state.
    Mutex m_mutex;

    /// Condition variable to wait for.
    ConditionVariable m_condition;

    /// Is true as long at the thread should continue.
    bool m_is_running = true;
};

NOTF_CLOSE_NAMESPACE
