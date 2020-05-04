#include "notf/app/timer_pool.hpp"

#include "notf/meta/integer.hpp"

// helper =========================================================================================================== //

namespace {
NOTF_USING_NAMESPACE;

size_t check_buffer_size(const size_t buffer_size) {
    if (buffer_size == 0) { NOTF_THROW(ValueError, "TimerPool buffer size must not be zero"); }
    if (!is_power_of_two(buffer_size)) {
        NOTF_THROW(ValueError, "TimerPool buffer size must be a power of two, not {}", buffer_size);
    }
    return buffer_size;
}

} // namespace

NOTF_OPEN_NAMESPACE

// timer pool ======================================================================================================= //

namespace detail {

TimerPool::TimerPool(const size_t buffer_size)
    : m_buffer(check_buffer_size(buffer_size)), m_timer_thread(Thread(Thread::Kind::TIMER_POOL)) {
    m_timer_thread.run([& buffer = m_buffer] {
        fibers::mutex mutex;
        fibers::condition_variable while_running;
        Fiber([&buffer, &mutex, &while_running] {

            TimerPtr timer;
            while (fibers::channel_op_status::success == buffer.pop(timer)) {

                // each timer gets its own fiber to run on
                Fiber(fibers::launch::dispatch, [&, timer = std::move(timer)]() mutable {
                    // make sure there exist another shared_ptr that keeps the timer alive from outside the pool
                    const TimerWeakPtr weak_timer = timer;
                    if (!timer->is_detached()) {
                        timer.reset();
                        timer = weak_timer.lock();
                    }

                    while (timer && timer->is_active()) {
                        // fire right away if the next timeout has passed
                        const auto timeout = timer->get_next_timeout();
                        if (timeout <= get_now()) {
                            timer->fire();

                        } else {
                            // instead of just sleeping, all fibers wait on a condition variable
                            // this way, we can forcefully shut down all timers on destruction of the timer pool
                            std::unique_lock lock(mutex);
                            if (while_running.wait_until(
                                    lock, std::chrono::time_point_cast<std::chrono::steady_clock::duration>(timeout),
                                    [&buffer] { return NOTF_UNLIKELY(buffer.is_closed()); })) {
                                if (!timer->is_keeping_alive()) {
                                    return; // return early if the buffer has been closed
                                }
                            }
                        }

                        // re-check whether the user has already discarded the timer
                        if (!timer->is_detached()) {
                            timer.reset();
                            timer = weak_timer.lock();
                        }
                    }
                }).detach();
            }
            // notify all waiting timers that the buffer has been closed
            while_running.notify_all();
        }).join();
    });
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
