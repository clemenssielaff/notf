#pragma once

#include "notf/meta/numeric.hpp"
#include "notf/meta/singleton.hpp"
#include "notf/meta/smart_ptr.hpp"
#include "notf/meta/time.hpp"

#include "notf/common/fibers.hpp"
#include "notf/common/thread.hpp"

#include "notf/app/fwd.hpp"

NOTF_OPEN_NAMESPACE

// timer pool ======================================================================================================= //

namespace detail {

class TimerPool {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param buffer_size  Number of items in the timer buffer before `schedule` blocks.
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

    friend Accessor<TheTimerPool, detail::Application>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(TheTimerPool);

    // methods --------------------------------------------------------------------------------- //
protected:
    NOTF_CREATE_SMART_FACTORIES(TheTimerPool);

public:
    // forward all constructors
    using ScopedSingleton<detail::TimerPool>::ScopedSingleton;
};

// accessors ======================================================================================================== //

template<>
class Accessor<TheTimerPool, detail::Application> {
    friend detail::Application;

    /// Creates the ScopedSingleton holder instance of TheTimerPool.
    template<class... Args>
    static auto create(Args... args) {
        return TheTimerPool::_create_unique(std::forward<Args>(args)...);
    }
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

    /// If true, will keep the Timer alive even if there are no more owning references to it outside the TimerPool.
    bool is_detached() const noexcept { return m_is_detached; }

    /// Starts the
    void start(const bool detach = false) {
        if (State expected = State::UNSTARTED; m_state.compare_exchange_strong(expected, State::RUNNING)) {
            m_is_detached = detach;
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
    std::atomic_bool m_is_detached = false;
};

// one-shot timer =================================================================================================== //

/// Timer firing a single time at some point in the future.
template<class Lambda, class Timepoint>
auto OneShotTimer(Timepoint timeout, Lambda&& lambda) {

    struct OneShotTimerImpl : public Timer {
        OneShotTimerImpl(timepoint_t timeout, Lambda lambda) : Timer(), m_lambda(std::forward<Lambda>(lambda)) {
            _set_next_timeout(timeout);
        }

    private:
        void _fire() final {
            stop();
            std::invoke(m_lambda);
        }

        Lambda m_lambda;
    };

    return std::make_shared<OneShotTimerImpl>(std::chrono::time_point_cast<duration_t>(timeout),
                                              std::forward<Lambda>(lambda));
}

// interval timer =================================================================================================== //

/// Timer firing `n` times in a constant interval.
template<class Lambda, class Duration>
auto IntervalTimer(Duration interval, Lambda&& lambda, uint repetitions = Timer::infinite) {

    struct IntervalTimerImpl : public Timer {
        IntervalTimerImpl(duration_t interval, Lambda lambda, uint repetitions)
            : Timer(repetitions), m_interval(interval), m_lambda(std::forward<Lambda>(lambda)) {
            _set_next_timeout(now() + m_interval);
        }

    private:
        void _fire() final {
            _set_next_timeout(now() + m_interval);
            std::invoke(m_lambda);
        }

        const duration_t m_interval;
        Lambda m_lambda;
    };

    return std::make_shared<IntervalTimerImpl>(std::chrono::duration_cast<duration_t>(interval),
                                               std::forward<Lambda>(lambda), repetitions);
}

// variable timer =================================================================================================== //

/// Timer firing `n` times with a variable timeout in between.
/// The Variable func must take no arguments to run and produce a `duration_t` value every time.
template<class Lambda, class VariableFunc>
auto VariableTimer(Lambda&& lambda, VariableFunc&& func, uint repetitions = Timer::infinite) {

    struct VariableTimerImpl : public Timer {
        VariableTimerImpl(Lambda lambda, VariableFunc func, uint repetitions)
            : Timer(repetitions)
            , m_lambda(std::forward<Lambda>(lambda))
            , m_variable_func(std::forward<VariableFunc>(func)) {
            _set_next_timeout(now() + m_variable_func());
        }

    private:
        void _fire() final {
            _set_next_timeout(now() + m_variable_func());
            std::invoke(m_lambda);
        }

        Lambda m_lambda;
        VariableFunc m_variable_func;
    };

    return std::make_shared<VariableTimerImpl>(std::forward<Lambda>(lambda), std::forward<VariableFunc>(func),
                                               repetitions);
}

NOTF_CLOSE_NAMESPACE
