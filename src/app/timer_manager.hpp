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
    std::mutex m_mutex;

    /// Condition variable to wait for.
    std::condition_variable m_condition;

    /// Is true as long at the thread is alive.
    bool m_is_running = true;
};

namespace detail {} // namespace detail

//====================================================================================================================//

/// The Timer class is fully thread-safe.
/// Timers are manged through shared pointers. If a Timer is scheduled when its last user-held shared_ptr goes out of
/// scope, it will execute once more before being removed from the SchedulingManager. This way, we can create anonymous
/// one-shot Timer that are never held by the user.
///
/// This is a NOTF_SAFETY_OFF class, meaning it should only be available for internal use and the experienced user,
/// because callbacks called from a Timer are executed synchronously and only call code that is thread safe.
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// Callback signature.
    using Callback = std::function<void()>;

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Default Constructor.
    /// @param callback     Function called when this Timer times out.
    Timer(Callback callback) : m_callback(std::move(callback)) {}

public:
    NOTF_NO_COPY_OR_ASSIGN(Timer)

    /// Factory.
    /// @param callback     Function called when this Timer times out.
    static TimerPtr create(Callback callback) { return NOTF_MAKE_SHARED_FROM_PRIVATE(Timer, std::move(callback)); }

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// @param duration Wait time until the Callback from now.
    /// @param callback Callback function.
    static void one_shot(const duration_t interval, Callback callback)
    {
        one_shot(TimerManager::_now() + interval, std::move(callback));
    }

    /// Schedules a one-off Timer to fire the given Callback at a certain point in the future.
    /// @param timeout  Point in time to run the callback.
    /// @param callback Callback function.
    static void one_shot(const time_point_t timeout, Callback callback)
    {
        if (timeout <= TimerManager::_now()) {
            return callback();
        }
        create(std::move(callback))->start(timeout);
    }

    /// Tests whether the Timer is currently active or not.
    bool is_active()
    {
        std::unique_lock<Mutex> guard(m_mutex);
        return _is_active();
    }

    /// Whether or not the Timer repeats automatically or not.
    bool is_repeating()
    {
        std::unique_lock<Mutex> guard(m_mutex);
        return _is_repeating();
    }

    /// Starts the Timer with unchanged settings.
    /// If the Timer is already scheduled, this method stops before starting again.
    /// If this is the first Time the timer has started, it will simply call the Callback once.
    /// If the Timer was already started with an interval once and stopped again, this method will run with the same
    /// interval.
    void start();

    /// Starts the Timer to fire once at a given point in the future.
    /// If the Timer is already scheduled, this method stops before starting again.
    /// If the time point is not in the future, the Callback is called immediately.
    /// @param timeout  Point in time to call the Callback.
    void start(const time_point_t timeout);

    /// Starts with a given interval, either as a one-shot or continuous Timer.
    /// If the Timer is already running, restarts it with the given values.
    /// @param interval     Interval wait time of the Timer, starts from now.
    /// @param is_repeating Whether or not the Timer repeats automatically or not (default is false).
    void start(const duration_t interval, const bool is_repeating = true);

    /// Stops the Timer, if it is active.
    void stop();

private:
    /// Tests whether the Timer is currently active or not (does not lock the mutex).
    bool _is_active() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_next_timeout != time_point_t();
    }

    /// Whether or not the Timer repeats automatically or not (does not lock the mutex).
    bool _is_repeating() const
    {
        NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
        return m_interval != duration_t(0);
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Time when the timer fires next.
    time_point_t m_next_timeout = time_point_t();

    /// Time between firing, is zero if this is a one-shot Timer.
    duration_t m_interval = duration_t(0);

    /// Function called when this Timer times out.
    Callback m_callback;

    /// Mutex protecting this Timer.
    Mutex m_mutex;
};

NOTF_CLOSE_NAMESPACE
