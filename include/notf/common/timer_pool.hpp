#pragma once

#include <forward_list>

#include "../meta/numeric.hpp"
#include "../meta/pointer.hpp"
#include "../meta/smart_factories.hpp"
#include "../meta/time.hpp"
#include "./common.hpp"
#include "./mutex.hpp"
#include "./thread.hpp"

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE
NOTF_USING_NAMESPACE;

/// A single thread running 0-n Timer instances used to trigger timed events like animations.
class TheTimerPool {
    friend class Timer;
    friend class OneShotTimer;
    friend class IntervalTimer;
    friend class VariableTimer;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Default Constructor.
    TheTimerPool();

    /// Static (private) function holding the actual TimerPool instance.
    static TheTimerPool& _instance()
    {
        static TheTimerPool instance;
        return instance;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(TheTimerPool);

    /// The singleton Timer Pool instance.
    static TheTimerPool& get() { return _instance(); }

    /// Destructor.
    ~TheTimerPool();

private:
    /// Worker thread method.
    void _run();

    /// Schedules a new Timer.
    /// @param timer    Timer to schedule.
    void _schedule(TimerPtr timer);

    /// Unschedules an existing Timer.
    /// @param timer    Timer to unschedule.
    void _unschedule(const valid_ptr<Timer*> timer);

    /// Current time point.
    static timepoint_t _now() { return std::chrono::time_point_cast<duration_t>(clock_t::now()); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// All registered Timers, ordered by their next timeout.
    std::forward_list<TimerPtr> m_timer;

    /// Worker thread.
    ScopedThread m_thread;

    /// Mutex guarding the Timer thread's state.
    Mutex m_mutex;

    /// Condition variable to wait for.
    ConditionVariable m_condition;

    /// Is true as long at the thread is alive.
    bool m_is_running = true;
};

// ================================================================================================================== //

/// The Timer class is fully thread-safe.
/// Timers are manged through shared pointers. If a Timer is scheduled when its last user-held shared_ptr goes out of
/// scope, it will execute once more before being removed from the TimerPool. This way, we can create anonymous one-shot
/// Timers that are never held by the user.
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TheTimerPool;

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Callback signature.
    using Callback = std::function<void()>;

    // methods ------------------------------------------------------------------------------------------------------ //
protected:
    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    Timer(Callback callback, size_t repetitions = _infinity())
        : m_callback(std::move(callback)), m_times_left(repetitions)
    {}

public:
    NOTF_NO_COPY_OR_ASSIGN(Timer);

    /// Destructor.
    virtual ~Timer();

    /// Starts the Timer to fire once at a given point in the future.
    /// If the Timer is already running, it is restarted with the given value.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param timeout      Point in time to call the Callback.
    void start(timepoint_t timeout);

    /// Stops the Timer, if it is active.
    void stop();

protected:
    /// Time to wait between this Timer fires.
    virtual duration_t _interval() const = 0;

    /// Tests whether the Timer is currently active or not.
    bool _is_active() const { return m_next_timeout != timepoint_t(); }

    /// Tests whether this Timer repeats infinitely.
    bool _is_infinite() const { return m_times_left == _infinity(); }

    /// If the user wants to have the Callback repeated this much, it might as well be infinity.
    constexpr static size_t _infinity() { return max_value<size_t>(); }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// Function called when this Timer times out.
    Callback m_callback;

    /// Time when the timer fires next.
    timepoint_t m_next_timeout = timepoint_t();

    /// How often the Timer will fire, if it is continuous.
    size_t m_times_left;
};

// ================================================================================================================== //

/// Fires once, after a given delay.
class OneShotTimer : public Timer {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_CREATE_SMART_FACTORIES(OneShotTimer);

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    OneShotTimer(Callback callback) : Timer(std::move(callback), 1) {}

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static OneShotTimerPtr create(Callback callback) { return _create_shared(std::move(callback)); }

public:
    /// Destructor.
    ~OneShotTimer() override;

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param timeout      Point in time to run the callback.
    /// @param callback     Callback function.
    static void create(const timepoint_t timeout, Callback callback)
    {
        if (timeout <= TheTimerPool::_now()) {
            callback();
            return;
        }
        create(std::move(callback))->start(timeout);
    }

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param duration     Wait time until the Callback from now.
    /// @param callback     Callback function.
    static void create(const duration_t interval, Callback callback)
    {
        create(TheTimerPool::_now() + interval, std::move(callback));
    }

protected:
    /// Time to wait between this Timer fires.
    duration_t _interval() const override { return duration_t(0); } // is never actually called
};

// ================================================================================================================== //

/// An IntervalTimer fires continuously with a fixed interval.
class IntervalTimer : public Timer {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_CREATE_SMART_FACTORIES(IntervalTimer);

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    /// @param interval     Interval wait time of the Timer.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    IntervalTimer(Callback callback, duration_t interval, size_t repetitions)
        : Timer(std::move(callback), repetitions), m_interval(std::move(interval))
    {}

public:
    /// Destructor.
    ~IntervalTimer() override;

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    /// @param interval     Interval wait time of the Timer.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    static IntervalTimerPtr
    create(Callback callback, duration_t interval = duration_t(0), size_t repetitions = _infinity())
    {
        return _create_shared(std::move(callback), std::move(interval), repetitions);
    }

    /// Starts with a given interval as a continuous Timer.
    /// If the Timer is already running, it is restarted with the given values.
    /// If the interval is zero, the Callback is fired immediately on this thread, as many times as the repetitions
    /// parameter says, except if it is infinite - then the Callback is executed just once (for your convenience).
    /// @param interval     Interval wait time of the Timer, starts from now.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    void start(duration_t interval, size_t repetitions);
    void start(size_t repetitions) { start(m_interval, repetitions); }
    void start(duration_t interval) { start(interval, m_times_left); }
    void start() { start(m_interval, m_times_left); }

private:
    /// Time to wait between this Timer fires.
    duration_t _interval() const override { return m_interval; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Time between firing.
    duration_t m_interval = duration_t(0);
};

// ================================================================================================================== //

/// An Variable fires continuously with an interval determined through a user-defined lambda.
class VariableTimer : public Timer {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Interval function signature.
    using IntervalFunction = std::function<duration_t()>;

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    NOTF_CREATE_SMART_FACTORIES(VariableTimer);

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    VariableTimer(Callback callback) : Timer(std::move(callback)) {}

public:
    /// Destructor.
    ~VariableTimer() override;

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static VariableTimerPtr create(Callback callback) { return _create_shared(std::move(callback)); }

    /// Starts with a given interval function.
    /// If the Timer is already running, it is restarted with the new function.
    /// The Timer is always scheduled, even if the given function returns zero.
    /// @param function     Function used to determine the next interval of this Timer.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    void start(IntervalFunction function, size_t repetitions = _infinity());

private:
    /// Time to wait between this Timer fires.
    duration_t _interval() const override { return m_function(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Function used to determine the next interval of this Timer.
    IntervalFunction m_function;
};

NOTF_CLOSE_NAMESPACE
