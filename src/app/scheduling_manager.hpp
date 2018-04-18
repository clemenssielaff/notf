#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>

#include "app/forwards.hpp"
#include "common/mutex.hpp"
#include "common/thread.hpp"

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

/// A single thread running 0-n Timer instances used to trigger timed events like animations.
///
/// WARNING
/// You must not use Timers to trigger changes in a Scene hierarchy!
/// Only use it to modify Properties or create Events.
class SchedulingManager {
    friend class Timer;

    // types ---------------------------------------------------------------------------------------------------------//
private:
    /// Most precise but steady clock.
    using clock_t = std::conditional_t<std::chrono::high_resolution_clock::is_steady, // if it is steady
                                       std::chrono::high_resolution_clock,            // use the high-resolution clock,
                                       std::chrono::steady_clock>;                    // otherwise use the steady clock

    /// The famous "flicks" duration type, described in length at:
    ///     https://github.com/OculusVR/Flicks/blob/master/flicks.h
    /// BSD License:
    ///     https://github.com/OculusVR/Flicks/blob/master/LICENSE
    using duration_t = std::chrono::duration<std::chrono::nanoseconds::rep, std::ratio<1, 705600000>>;

    /// Point in time.
    using time_point_t = std::chrono::time_point<clock_t, duration_t>;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default Constructor.
    SchedulingManager() = default;

    /// Destructor.
    ~SchedulingManager();

private:
    /// Worker thread method.
    void _run();

    /// Current time point.
    static time_point_t _now() { return std::chrono::time_point_cast<duration_t>(clock_t::now()); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All registered Timers, ordered by their next timeout.
    std::list<TimerPtr> m_timer;

    /// Worker thread.
    ScopedThread m_thread;

    /// Mutex guarding the RenderThread's state.
    Mutex m_mutex;

    /// Condition variable to wait for.
    std::condition_variable m_condition;

    /// Is true as long at the thread is alive.
    bool m_is_running = false;
};

//====================================================================================================================//

class Timer : public std::enable_shared_from_this<Timer> {
    friend class SchedulingManager;

    /// Callback signature.
    using Callback = std::function<void()>;

    /// Duration type.
    using duration_t = SchedulingManager::duration_t;

    /// Point in time.
    using time_point_t = SchedulingManager::time_point_t;

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
    std::shared_ptr<Timer> create(Callback callback)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(Timer, std::move(callback));
    }

    /// Tests whether the Timer is currently active or not.
    bool is_active() const { return m_next_timeout != time_point_t(); }

    /// Whether or not the Timer repeats automatically or not.
    bool is_repeating() const { return m_interval != duration_t(0); }

    /// @{
    /// Starts the Timer.
    /// If the Timer is already running, restarts it with the given values.
    /// @param interval     Interval wait time of the Timer.
    /// @param is_repeating Whether or not the Timer repeats automatically or not (default is false).
    void start()
    {
        if (m_interval == duration_t(0)) {
            return m_callback();
        }
        start(m_interval, true);
    }
    void start(const duration_t interval, const bool is_repeating = false);
    /// @}

    /// Stops the Timer, if it is active.
    void stop();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Time when the timer fires next.
    time_point_t m_next_timeout = time_point_t();

    /// Time between firing, is zero if this is a one-shot Timer.
    duration_t m_interval = duration_t(0);

    /// Function called when this Timer times out.
    Callback m_callback;
};

NOTF_CLOSE_NAMESPACE
