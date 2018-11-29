#include "notf/app/event/scheduler.hpp"

#include "notf/app/event/event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

Scheduler::Scheduler(const size_t buffer_size)
    : m_event_queue(buffer_size), m_event_handler(Thread(Thread::Kind::EVENT)) {

    // event handling loop
    m_event_handler.run([&] {
        fibers::fiber event_fiber([&] {
            AnyEventPtr event;
            while (fibers::channel_op_status::success == m_event_queue.pop(event)) {
                event->run();
            }
        });
        event_fiber.join();
    });
}

NOTF_CLOSE_NAMESPACE
