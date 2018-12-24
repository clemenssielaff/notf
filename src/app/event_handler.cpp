#include "notf/app/event_handler.hpp"

#include "notf/meta/integer.hpp"

#include "notf/app/render_manager.hpp"

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
    : m_event_queue(check_buffer_size(buffer_size)), m_event_handler(Thread(Thread::Kind::EVENT)) {}

EventHandler::~EventHandler() {
    m_event_queue.close();
    m_event_handler.join();
}

void EventHandler::_start(RecursiveMutex& ui_mutex) {
    m_event_handler.run([& queue = m_event_queue, &ui_mutex] {
        // take over as the UI thread
        std::unique_lock<RecursiveMutex> ui_lock(ui_mutex);
        NOTF_ASSERT(this_thread::is_the_ui_thread());

        // start the event handling loop
        Fiber event_fiber([&queue] {
            float weight = next_after(0.f);
            auto the_render_manager = TheRenderManager();
            AnyEventPtr event;
            fibers::channel_op_status status = fibers::channel_op_status::success;

            status = queue.pop(event); // wait for the first event
            while (fibers::channel_op_status::closed != status) {
                while (fibers::channel_op_status::success == status) {
                    NOTF_ASSERT(event);
                    weight += event->get_weight();

                    // each event gets its own fiber to execute on, in case it decides to block
                    // "dispatch" means: execute immediately
                    Fiber(fibers::launch::dispatch, [event = std::move(event)] { event->run(); }).detach();
                    // TODO: what about event handlers that block application shutdown?

                    // render the next frame immediately, once enough "weight" has been handled
                    if (weight >= 1.f) {
                        weight = 0;
                        the_render_manager->render();
                    }

                    // try to get the next event
                    status = queue.try_pop(event);
                }

                // always render a frame once the queue is empty
                if (fibers::channel_op_status::empty == status) {

                    // if the last event added enough weight to render a frame and no other events have been handled
                    // since, we do not need to render another frame here. Therefore we treat zero as a special case.
                    if (is_zero(weight)) { the_render_manager->render(); }
                    weight = next_after(0.f);

                    // wait for the next event or for the queue to close
                    status = queue.pop(event);
                }
            } // end of event loop
        });   // end of fiber
        event_fiber.join();
    });
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
