#include <atomic>
#include <iostream>
#include <unordered_set>

#include "notf/meta/numeric.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/time.hpp"

#include "notf/common/fibers.hpp"
#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

NOTF_OPEN_NAMESPACE

class TimerPool;
NOTF_DECLARE_SHARED_POINTERS(class, Timer);

// any timer ======================================================================================================== //

class Timer {

    friend TimerPool;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Additional arguments on how a Timer should behave.
    struct Flags {

        /// If true, exceptions thrown during the Timer execution are ignored and the Timer will be rescheduled as if
        /// nothing happened. The last exception is still stored in the Timer instance, all but the last exception are
        /// lost (ignored).
        bool ignore_exceptions = false;

        /// If true, keeps the TimerPool alive, even if its destructor has been called.
        /// Using this flag, you can ensure that a Timer will fire before the Application is closed. On the other hand,
        /// it will prevent the whole Application from shutting down in an orderly fashion, so use only when you
        /// know what you are doing and are sure you need it.
        bool keep_alive = false;

        /// If true, this Timer will stay alife even if there is no more `TimerPtr` held outside of the TimerPool.
        /// Otherwise, removing the last TimerPtr to a timer on the outside will immediately stop the Timer.
        bool anonymous = false;
    };

    /// Special "repetitions" value denoting infinite repetitions.
    static constexpr uint infinity = max_value<uint>();

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Timer);

    /// Constructor.
    /// @param repetitions  Number of times the Timer should fire.
    /// @param flags         Additional arguments on how this Timer should behave.
    Timer(uint repetitions, Flags flags)
        : m_repetitions_left(repetitions), m_is_active(m_repetitions_left != 0), m_flags(flags) {}

    /// Virtual destructor.
    virtual ~Timer() = default;

    /// Whether or not the Timer is still active.
    bool is_active() const noexcept { return m_is_active; }

    /// Constant flags.
    const Flags& get_flags() const { return m_flags; }

    bool has_exception() const noexcept { return static_cast<bool>(m_exception); }

    void rethrow() {
        if (has_exception()) { std::rethrow_exception(m_exception); }
    }

    /// Stops the timer and prevents it from firing again.
    void stop() noexcept { m_is_active = false; }

private: // for Timer Pool
    virtual timepoint_t get_next_timeout() const noexcept = 0;

    /// Runs the callback stored in the Timer.
    void fire() {
        if (m_is_active) {
            try {
                _fire();
            }
            catch (...) {
                m_exception = std::current_exception();
                if (!m_flags.ignore_exceptions) { stop(); }
            }

            // stop if this was the last repetition
            if (m_repetitions_left != infinity && --m_repetitions_left == 0) { stop(); }
        }
    }

private:
    /// Implementation dependent fire method.
    virtual void _fire() = 0;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Exception thrown during execution.
    std::exception_ptr m_exception;

    /// Number of times that the Timer will fire left.
    uint m_repetitions_left;

    /// Whether or not the Timer is still active or if it has been stopped.
    std::atomic_bool m_is_active;

    /// Arguments passed into the constructor.
    const Flags m_flags;
};

// one-shot timer =================================================================================================== //

template<class Lambda>
auto OneShotTimer(timepoint_t timeout, Lambda&& lambda, uint repetitions = Timer::infinity, Timer::Flags flags = {}) {

    class OneShotTimerImpl : public Timer {

        // methods --------------------------------------------------------------------------------- //
    private:
        static Flags _modify_flags(Flags flags) {
            flags.anonymous = true;
            return flags;
        }

    public:
        OneShotTimerImpl(timepoint_t timeout, Lambda&& lambda, uint repetitions, Flags flags)
            : Timer(repetitions, _modify_flags(flags))
            , m_timeout(std::move(timeout))
            , m_lambda(std::forward<Lambda>(lambda)) {}

        timepoint_t get_next_timeout() const noexcept final { return m_timeout; }

    private:
        void _fire() final {
            stop();
            std::invoke(m_lambda);
        }

        // fields ---------------------------------------------------------------------------------- //
    private:
        /// Next time the lambda is executed.
        timepoint_t m_timeout;

        /// Lambda to execute on timeout.
        Lambda m_lambda;
    };

    return std::make_shared<OneShotTimerImpl>(timeout, std::forward<Lambda>(lambda), repetitions, flags);
}

// interval timer =================================================================================================== //

// template<class Lambda>
// auto IntervalTimer(timepoint_t timeout, Lambda&& lambda, size_t repetitions = 0, Timer::Args arguments = {}) {

//    class IntervalTimerImpl : public Timer {

//        // methods --------------------------------------------------------------------------------- //
//    public:
//        IntervalTimerImpl(timepoint_t timeout, Lambda&& lambda, Args arguments)
//            : Timer(std::move(arguments)), m_lambda(std::forward<Lambda>(lambda)), m_timeout(std::move(timeout)) {}

//        timepoint_t get_next_timeout() const noexcept final { return m_timeout; }

//    private:
//        void _fire() final {
//            stop();
//            std::invoke(m_lambda);
//        }

//        // fields ---------------------------------------------------------------------------------- //
//    private:
//        /// Lambda to execute on timeout.
//        Lambda m_lambda;

//        /// Next time the lambda is executed.
//        timepoint_t m_timeout;
//    };

//    return std::make_shared<OneShotTimerImpl>(timeout, std::forward<Lambda>(lambda), std::move(arguments));
//}

// variable timer =================================================================================================== //

// timer pool ======================================================================================================= //

class TimerPool {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param buffer_size  Number of items in the timer buffer before `schedule` blocks.
    TimerPool(size_t buffer_size = 32) : m_buffer(buffer_size) {
        m_timer_thread.run([this] {
            fibers::mutex mutex;
            Fiber([this, &mutex] {
                TimerPtr timer;
                while (fibers::channel_op_status::success == m_buffer.pop(timer)) {

                    // each timer gets its own fiber to run on
                    Fiber(fibers::launch::dispatch, [this, &mutex, timer = std::move(timer)]() mutable {
                        // make sure there exist another shared_ptr that keeps the timer alive from outside the pool
                        const TimerWeakPtr weak_timer = timer;
                        if (!timer->get_flags().anonymous) {
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
                                if (m_while_running.wait_until(
                                        lock,
                                        std::chrono::time_point_cast<std::chrono::steady_clock::duration>(timeout),
                                        [&] { return NOTF_UNLIKELY(m_buffer.is_closed()); })) {
                                    if (!timer->get_flags().keep_alive) {
                                        return; // return early if the buffer has been closed
                                    }
                                }
                            }

                            // re-check whether the user has already discarded the timer
                            if (!timer->get_flags().anonymous) {
                                timer.reset();
                                timer = weak_timer.lock();
                            }
                        }
                    }).detach();
                }
                m_while_running.notify_all();
            }).join();
        });
    }

    /// Destructor.
    ~TimerPool() { m_buffer.close(); }

    /// Schedules a new Timer in the Pool.
    void schedule(TimerPtr&& timer) { m_buffer.push(std::move(timer)); }

    // fields ---------------------------------------------------------------------------------- //
private:
    fibers::condition_variable m_while_running;

    /// MPMC queue buffering new Timers to be scheduled in the Pool.
    fibers::buffered_channel<TimerPtr> m_buffer;

    /// Thread running the Timer Fibers.
    Thread m_timer_thread;
};

NOTF_CLOSE_NAMESPACE

int main() {
    NOTF_USING_NAMESPACE;
    NOTF_USING_LITERALS;

    TimerPool pool;
    pool.schedule(OneShotTimer(now() + 1s, [] { std::cout << "derbe after 1 seconds" << std::endl; }));
    pool.schedule(OneShotTimer(now() + 2s, [] { std::cout << "derbe after 2 seconds" << std::endl; }));
    pool.schedule(OneShotTimer(now() + 3s, [] { std::cout << "derbe after 3 seconds" << std::endl; }));
    std::this_thread::sleep_for(1.2s);
    return 0;
}
