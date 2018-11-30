#include "notf/app/event/scheduler.hpp"

#include "notf/app/event/event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

Scheduler::Scheduler(const size_t buffer_size)
    : m_event_queue(buffer_size), m_event_handler(Thread(Thread::Kind::EVENT)) {

    // event handling loop
    m_event_handler.run([& queue = m_event_queue] {
        Fiber event_fiber([&] {
            AnyEventPtr event;
            while (fibers::channel_op_status::success == queue.pop(event)) {
                // each event gets its own fiber to execute on
                Fiber(fibers::launch::dispatch, [event = std::move(event)] { event->run(); }).detach();
            }
        });
        event_fiber.join();
    });
}

NOTF_CLOSE_NAMESPACE
