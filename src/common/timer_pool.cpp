#include "notf/common/timer_pool.hpp"

NOTF_OPEN_NAMESPACE

// timer pool ======================================================================================================= //

TimerPool::TimerPool(const size_t buffer_size) : m_buffer(buffer_size) {
    m_timer_thread.run([& buffer = m_buffer] {
        Fiber([&buffer] {
            fibers::mutex mutex;
            fibers::condition_variable while_running;

            TimerPtr timer;
            while (fibers::channel_op_status::success == buffer.pop(timer)) {

                // each timer gets its own fiber to run on
                Fiber(fibers::launch::dispatch, [&, timer = std::move(timer)]() mutable {
                    // make sure there exist another shared_ptr that keeps the timer alive from outside the pool
                    const TimerWeakPtr weak_timer = timer;
                    if (!timer->is_anonymous()) {
                        timer.reset();
                        timer = weak_timer.lock();
                    }

                    while (timer && timer->is_active()) {
                        // fire right away if the next timeout has passed
                        const auto timeout = timer->get_next_timeout();
                        if (timeout <= now()) {
                            timer->fire();

                        } else {
                            // instead of just waiting, all fibers wait on a condition variable
                            // this way, we can forcefully shut down all timers on destruction of the timer pool
                            std::unique_lock lock(mutex);
                            if (while_running.wait_until(
                                    lock, std::chrono::time_point_cast<std::chrono::steady_clock::duration>(timeout),
                                    [&] { return NOTF_UNLIKELY(buffer.is_closed()); })) {
                                if (!timer->is_keeping_alive()) {
                                    return; // return early if the buffer has been closed
                                }
                            }
                        }

                        // re-check whether the user has already discarded the timer
                        if (!timer->is_anonymous()) {
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

NOTF_CLOSE_NAMESPACE
