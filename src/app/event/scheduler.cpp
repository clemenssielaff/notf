#include "notf/app/event/scheduler.hpp"

#include "notf/app/event/key_events.hpp"
#include "notf/app/event/mouse_events.hpp"
#include "notf/app/event/window_events.hpp"

// event handler ==================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

} // namespace

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

Scheduler::Scheduler(const size_t buffer_size)
    : m_event_handler(Thread(Thread::Kind::EVENT)), m_event_queue(buffer_size) {

    // event handling loop
    m_event_handler.run([&]() {
        AnyEvent event;
        while (fibers::channel_op_status::success == m_event_queue.pop(event)) {}
    });
}

NOTF_CLOSE_NAMESPACE
