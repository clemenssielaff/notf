#include "notf/app/event/handler.hpp"

#include "notf/meta/integer.hpp"

#include "notf/app/event/event.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

size_t check_buffer_size(const size_t buffer_size) {
    if (buffer_size == 0) { NOTF_THROW(ValueError, "EventHandler buffer size must not be zero"); }
    if (!is_power_of_two(buffer_size)) {
        NOTF_THROW(ValueError, "EventHandler buffer size must be a power of two, not {}", buffer_size);
    }
    return buffer_size;
}

} // namespace

NOTF_OPEN_NAMESPACE

// event handler ==================================================================================================== //

namespace detail {

EventHandler::EventHandler(const size_t buffer_size)
    : m_event_queue(check_buffer_size(buffer_size)), m_event_handler(Thread(Thread::Kind::EVENT)) {

    // event handling loop
    m_event_handler.run([& queue = m_event_queue] {
        Fiber event_fiber([&] {
            AnyEventPtr event;
            while (fibers::channel_op_status::success == queue.pop(event)) {
                // each event gets its own fiber to execute on
                Fiber(fibers::launch::dispatch, [event = std::move(event)] { event->run(); }).detach();
                // TODO: what about event handlers that block application shutdown?
            }
        });
        event_fiber.join();
    });
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
