#pragma once

#include <condition_variable>
#include <forward_list>
#include <functional>

#include "app/forwards.hpp"
#include "common/assert.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"
#include "common/thread.hpp"
#include "common/time.hpp"

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

/// A single thread running 0-n Timer instances used to trigger timed events like animations.
///
/// WARNING
/// You must not use Timers to trigger changes in a Scene hierarchy!
/// Only use it to modify Properties or create Events.
class TimerManager {
    friend class Timer;
    friend class IntervalTimer;
    friend class VariableTimer;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default Constructor.
    TimerManager();

    /// Destructor.
    ~TimerManager();

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
    static time_point_t _now() { return std::chrono::time_point_cast<duration_t>(clock_t::now()); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All registered Timers, ordered by their next timeout.
    std::forward_list<TimerPtr> m_timer;

    /// Worker thread.
    ScopedThread m_thread;

    /// Mutex guarding the RenderThread's state.
    Mutex m_mutex;

    /// Condition variable to wait for.
#ifdef NOTF_DEBUG
    std::condition_variable_any m_condition;
#else
    std::condition_variable m_condition;
#endif

    /// Is true as long at the thread is alive.
    bool m_is_running = true;
};

//====================================================================================================================//

/// The Timer class is fully thread-safe.
/// Timers are manged through shared pointers. If a Timer is scheduled when its last user-held shared_ptr goes out of
/// scope, it will execute once more before being removed from the SchedulingManager. This way, we can create anonymous
/// one-shot Timer that are never held by the user.
///
/// This is a NOTF_SAFETY_OFF class, meaning it should only be available for internal use and the experienced user,
/// because callbacks called from a Timer are executed synchronously and only call code that is thread safe.
class Timer : public std::enable_shared_from_this<Timer> {
    friend class notf::TimerManager;

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    /// Callback signature.
    using Callback = std::function<void()>;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    Timer(Callback callback) : m_callback(std::move(callback)) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(Timer)

    /// Destructor.
    virtual ~Timer();

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static TimerPtr create(Callback callback) { return NOTF_MAKE_SHARED_FROM_PRIVATE(Timer, std::move(callback)); }

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param duration     Wait time until the Callback from now.
    /// @param callback     Callback function.
    static void one_shot(const duration_t interval, Callback callback)
    {
        one_shot(TimerManager::_now() + interval, std::move(callback));
    }

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param timeout      Point in time to run the callback.
    /// @param callback     Callback function.
    static void one_shot(const time_point_t timeout, Callback callback)
    {
        if (timeout <= TimerManager::_now()) {
            return callback();
        }
        create(std::move(callback))->start(timeout);
    }

    /// Starts the Timer to fire once at a given point in the future.
    /// If the Timer is already running, it is restarted with the given value.
    /// If the time point is not in the future, the Callback is called immediately on this thread.
    /// @param timeout      Point in time to call the Callback.
    void start(const time_point_t timeout);

    /// Stops the Timer, if it is active.
    void stop();

protected:
    /// Time to wait between this Timer fires.
    virtual duration_t _interval() const { return duration_t(0); }

    /// Tests whether the Timer is currently active or not.
    bool _is_active() const { return m_next_timeout != time_point_t(); }

    /// Tests whether this Timer repeats infinitely.
    bool _is_infinite() const { return m_times_left == _infinity(); }

    /// If the user wants to have the Callback repeated this much, it might as well be infinity.
    constexpr static size_t _infinity() { return std::numeric_limits<size_t>::max(); }

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// Function called when this Timer times out.
    Callback m_callback;

    /// Time when the timer fires next.
    time_point_t m_next_timeout = time_point_t();

    /// How often the Timer will fire, if it is continuous.
    /// A value of zero is used for infinite
    size_t m_times_left = 0;
};

//====================================================================================================================//

/// An IntervalTimer fires continuously with a fixed interval.
class IntervalTimer : public Timer {

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    IntervalTimer(Callback callback) : Timer(std::move(callback)) {}

public:
    /// Destructor.
    ~IntervalTimer() override;

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static IntervalTimerPtr create(Callback callback)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(IntervalTimer, std::move(callback));
    }

    /// Starts with a given interval as a continuous Timer.
    /// If the Timer is already running, it is restarted with the given values.
    /// If the interval is zero, the Callback is fired immediately on this thread, as many times as the repetitions
    /// parameter says, except if it is infinite - then the Callback is executed just once.
    /// @param interval     Interval wait time of the Timer, starts from now.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    void start(const duration_t interval, const size_t repetitions = _infinity());

private:
    /// Time to wait between this Timer fires.
    duration_t _interval() const override { return m_interval; }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Time between firing.
    duration_t m_interval = duration_t(0);
};

//====================================================================================================================//

/// An Variable fires continuously with an interval determined through a user-defined lambda.
class VariableTimer : public Timer {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Interval function signature.
    using IntervalFunction = std::function<duration_t()>;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Value Constructor.
    /// @param callback     Function called when this Timer times out.
    VariableTimer(Callback callback) : Timer(std::move(callback)) {}

public:
    /// Destructor.
    ~VariableTimer() override;

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static VariableTimerPtr create(Callback callback)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(VariableTimer, std::move(callback));
    }

    /// Starts with a given interval function.
    /// If the Timer is already running, it is restarted with the new function.
    /// The Timer is always scheduled, even if the given function returns zero.
    /// @param function     Function used to determine the next interval of this Timer.
    /// @param repetitions  How often the Timer should fire. Default is infinite.
    void start(IntervalFunction function, const size_t repetitions = _infinity());

private:
    /// Time to wait between this Timer fires.
    duration_t _interval() const override { return m_function(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Function used to determine the next interval of this Timer.
    IntervalFunction m_function;
};

NOTF_CLOSE_NAMESPACE
