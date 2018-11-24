#pragma once

#include "notf/common/fibers.hpp"
#include "notf/common/thread_pool.hpp"

#include "notf/app/event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

class EventHandler {
    // types ----------------------------------------------------------------------------------- //
private:
    // methods --------------------------------------------------------------------------------- //
public:
    // fields ---------------------------------------------------------------------------------- //
private:
    Thread m_event_thread;
    ThreadPool m_worker_pool;
};

NOTF_CLOSE_NAMESPACE
