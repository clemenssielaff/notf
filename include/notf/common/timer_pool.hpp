#pragma once

#include "notf/meta/numeric.hpp"
#include "notf/meta/singleton.hpp"
#include "notf/meta/time.hpp"

#include "notf/common/fibers.hpp"
#include "notf/common/thread.hpp"

NOTF_OPEN_NAMESPACE

// timer pool ======================================================================================================= //

namespace detail {

class TimerPool {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param buffer_size  Number of items in the timer buffer before `schedule` blocks.
    ///                     Must be a power of two.
    /// @throws ValueError  If the buffer size is zero or not a power of two.
    TimerPool(size_t buffer_size = 32);

    /// Destructor.
    /// Automatically closes the pool and shuts down all running Timers (unless they are "keep-alive").
    ~TimerPool() {
        m_buffer.close();
        m_timer_thread.join();
    }

    /// Schedules a new Timer in the Pool.
    /// @param timer    Timer to schedule.
    void schedule(TimerPtr timer) { m_buffer.push(std::move(timer)); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// MPMC queue buffering new Timers to be scheduled in the Pool.
    fibers::buffered_channel<TimerPtr> m_buffer;

    /// Thread running the Timer Fibers.
    Thread m_timer_thread;
};

} // namespace detail

// the timer pool =================================================================================================== //

class TheTimerPool : public ScopedSingleton<detail::TimerPool> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Base type.
    using super_t = ScopedSingleton<detail::TimerPool>;

    // methods --------------------------------------------------------------------------------- //
public:
    using super_t::super_t; // use baseclass' constructors
};

// timer ============================================================================================================ //

/// Timer Baseclass.
class Timer : public std::enable_shared_from_this<Timer> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Special "repetitions" value denoting infinite repetitions.
    static constexpr uint infinite = max_value<uint>();

    enum class State : uchar {
        UNSTARTED,
        RUNNING,
        FINISHED,
    };

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Constructor.
    /// @param repetitions  Number of times that the Timer will fire left.
    Timer(const uint repetitions = infinite)
        : m_repetitions_left(repetitions), m_state(m_repetitions_left != 0 ? State::UNSTARTED : State::FINISHED) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(Timer);

    /// Virtual destructor.
    virtual ~Timer() = default;

    /// Whether or not the Timer is still active.
    bool is_active() const noexcept { return m_state.load() == State::RUNNING; }

    /// @{
    /// If false, stops timer on the first exceptions, otherwise keeps going.
    bool is_ignoring_exceptions() const noexcept { return m_ignore_exceptions; }
    void set_ignore_exceptions(const bool value = true) noexcept { m_ignore_exceptions = value; }
    /// @}

    /// @{
    /// If true, will keep the TimerPool alive until the Timer has finished on its own.
    bool is_keeping_alive() const noexcept { return m_keep_alive; }
    void set_keep_alive(const bool value = true) noexcept { m_keep_alive = value; }
    /// @}

    /// @{
    /// If true, will keep the Timer alive even if there are no more owning references to it outside the TimerPool.
    bool is_anonymous() const noexcept { return m_anonymous; }
    void set_anonymous(const bool value = true) noexcept { m_anonymous = value; }
    /// @}

    /// Starts the
    void start() {
        if (State expected = State::UNSTARTED; m_state.compare_exchange_strong(expected, State::RUNNING)) {
            TheTimerPool()->schedule(shared_from_this());
        }
    }

    /// Stops the timer and prevents it from firing again.
    void stop() noexcept {
        State expected = State::RUNNING;
        m_state.compare_exchange_strong(expected, State::FINISHED);
    }

    /// Next time the lambda is executed.
    timepoint_t get_next_timeout() const noexcept { return m_next_timeout; }

    /// Checks if the Timer's callback threw an exception during its last execution.
    bool has_exception() const noexcept { return static_cast<bool>(m_exception); }

    /// If the Timer has a stored exception, this will re-throw it.
    void rethrow() {
        if (has_exception()) { std::rethrow_exception(m_exception); }
    }

    /// Runs the callback stored in the Timer.
    void fire() {
        if (is_active()) {
            try {
                _fire();
            }
            catch (...) {
                m_exception = std::current_exception();
                if (!m_ignore_exceptions) { stop(); }
            }

            // stop if this was the last repetition
            if (m_repetitions_left != infinite && --m_repetitions_left == 0) { stop(); }
        }
    }

protected:
    /// Lets the implementations set the next timeout for this Timer.
    void _set_next_timeout(timepoint_t next_timeout) {
        if (next_timeout < m_next_timeout.load()) {
            NOTF_THROW(LogicError, "The next timeout of a Timer cannot be earlier than the last");
        }
        m_next_timeout.store(next_timeout);
    }

private:
    /// Implementation dependent fire method.
    virtual void _fire() = 0;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Exception thrown during execution.
    std::exception_ptr m_exception;

    /// Next time the lambda is executed.
    std::atomic<timepoint_t> m_next_timeout;

    /// Number of times that the Timer will fire left.
    uint m_repetitions_left = infinite;

    /// Whether or not the Timer is unstarted, still active or if it has been stopped.
    std::atomic<State> m_state = State::UNSTARTED;

    /// If true, exceptions thrown during the Timer execution are ignored and the Timer will be rescheduled as if
    /// nothing happened. The last exception is still stored in the Timer instance, all but the last exception are
    /// lost (ignored).
    std::atomic_bool m_ignore_exceptions = false;

    /// If true, keeps the TimerPool alive, even if its destructor has been called.
    /// Using this flag, you can ensure that a Timer will fire before the Application is closed. On the other hand,
    /// it will prevent the whole Application from shutting down in an orderly fashion, so use only when you
    /// know what you are doing and are sure you need it.
    std::atomic_bool m_keep_alive = false;

    /// If true, this Timer will stay alife even if there is no more `TimerPtr` held outside of the TimerPool.
    /// Otherwise, removing the last TimerPtr to a timer on the outside will immediately stop the Timer.
    std::atomic_bool m_anonymous = false;
};

// one-shot timer =================================================================================================== //

/// Timer firing a single time at some point in the future.
template<class Lambda, class Timepoint>
auto OneShotTimer(Timepoint timeout, Lambda&& lambda) {
    static const Lambda factory_lambda = std::forward<Lambda>(lambda);

    struct OneShotTimerImpl : public Timer {
        OneShotTimerImpl(timepoint_t timeout) : Timer() { _set_next_timeout(timeout); }

    private:
        void _fire() final {
            stop();
            std::invoke(factory_lambda);
        }
    };

    return std::make_shared<OneShotTimerImpl>(std::chrono::time_point_cast<duration_t>(timeout));
}

// interval timer =================================================================================================== //

/// Timer firing `n` times in a constant interval.
template<class Lambda, class Duration>
auto IntervalTimer(Duration interval, Lambda&& lambda, uint repetitions = Timer::infinite) {
    static const Lambda factory_lambda = std::forward<Lambda>(lambda);
    static const duration_t factory_interval = std::chrono::duration_cast<duration_t>(interval);

    struct IntervalTimerImpl : public Timer {
        IntervalTimerImpl(uint repetitions) : Timer(repetitions) { _set_next_timeout(now() + factory_interval); }

    private:
        void _fire() final {
            _set_next_timeout(now() + factory_interval);
            std::invoke(factory_lambda);
        }
    };

    return std::make_shared<IntervalTimerImpl>(repetitions);
}

// variable timer =================================================================================================== //

/// Timer firing `n` times with a variable timeout in between.
/// The Variable func must take no arguments to run and produce a `duration_t` value every time.
template<class Lambda, class VariableFunc>
auto VariableTimer(Lambda&& lambda, VariableFunc&& func, uint repetitions = Timer::infinite) {
    static const Lambda factory_lambda = std::forward<Lambda>(lambda);
    static const VariableFunc variable_func = std::forward<VariableFunc>(func);

    struct VariableTimerImpl : public Timer {
        VariableTimerImpl(uint repetitions) : Timer(repetitions) { _set_next_timeout(now() + variable_func()); }

    private:
        void _fire() final {
            _set_next_timeout(now() + variable_func());
            std::invoke(factory_lambda);
        }
    };

    return std::make_shared<VariableTimerImpl>(repetitions);
}

NOTF_CLOSE_NAMESPACE
