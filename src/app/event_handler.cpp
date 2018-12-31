#include "notf/app/event_handler.hpp"

#include "notf/meta/integer.hpp"
#include "notf/meta/log.hpp"

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
    m_event_handler.run([this, &ui_mutex] {
        // take over as the UI thread
        std::unique_lock<RecursiveMutex> ui_lock(ui_mutex);
        NOTF_ASSERT(this_thread::is_the_ui_thread());

        // start the event handling loop
        Fiber event_fiber([this] {
            size_t counter = 0;
            double weight = 0;
            auto the_render_manager = TheRenderManager();
            AnyEventPtr event;
            fibers::channel_op_status status = fibers::channel_op_status::success;

            status = m_event_queue.pop(event); // wait for the first event
            while (fibers::channel_op_status::closed != status) {
                while (fibers::channel_op_status::success == status) {

                    // each event gets its own fiber to execute on, in case it decides to block
                    // "dispatch" means: execute immediately
                    if (event) {
                        Fiber(fibers::launch::dispatch, [this, event = std::move(event), &counter, &weight] {
                            try {
                                event->run();
                                ++counter;
                                weight += max(0.001f, event->get_weight());

                                // We don't know if the event function is blocking or not. If it isn't, it will
                                // immediately add its weight and cause a new frame to render, if required.
                                // However, if it blocks, we *still* want to check if we need to render  a new frame.
                                // Therefore we force another run of the event loop just to be sure.
                                schedule(nullptr);
                                // TODO: what about event handlers that block application shutdown?
                            }
                            catch (const std::exception& exception) {
                                NOTF_LOG_CRIT("Failure during event handling: \"{}\"", exception.what());
                            }
                        }).detach();
                    }

                    // render the next frame immediately, once enough "weight" has been handled
                    if (weight >= 1.) {
                        counter = 0;
                        weight = 0;
                        the_render_manager->render();
                    }

                    // try to get the next event
                    status = m_event_queue.try_pop(event);
                }

                // always render a frame once the queue is empty
                if (fibers::channel_op_status::empty == status) {

                    // if the last event added enough weight to render a frame and no other events have been handled
                    // since, we do not need to render another frame here.
                    if (counter > 0) {
                        counter = 0;
                        weight = 0;
                        the_render_manager->render();
                    }

                    // wait for the next event or for the queue to close
                    status = m_event_queue.pop(event);
                }
            } // end of event loop
        });   // end of fiber
        event_fiber.join();
    });
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
