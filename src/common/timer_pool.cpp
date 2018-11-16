#include "notf/common/timer_pool.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

TheTimerPool::TheTimerPool() { m_thread.run(&TheTimerPool::_run, this); }

TheTimerPool::~TheTimerPool()
{
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        m_is_running = false;
    }
    m_condition.notify_one();
    m_thread = {}; // blocks until the thread has joined
}

void TheTimerPool::_run()
{
    TimerPtr next_timer;
    while (true) {
        {
            std::unique_lock<Mutex> lock(m_mutex);

            // re-schedule the last timer, if it is repeating AND if this is not the last reference to the timer
            // otherwise we could end up with timer that could never be stopped
            if ((next_timer.use_count() > 1) && (next_timer->m_times_left > 0)) {
                next_timer->m_next_timeout += next_timer->_interval();
                _schedule(std::move(next_timer));
            }

            next_timer.reset();
            while (!next_timer) {
                // wait until there is a timeout to wait for
                if (m_timer.empty()) {
                    m_condition.wait(lock, [&] { return !m_timer.empty() || !m_is_running; });
                }

                // stop the thread
                if (!m_is_running) { return; }
                NOTF_ASSERT(!m_timer.empty());

                // wait for the next timeout
                const auto next_timeout = m_timer.front()->m_next_timeout;
                if (next_timeout <= _now()) {
                    next_timer = std::move(m_timer.front());
                    break;
                }
                m_condition.wait_until(lock, next_timeout, [&] {
                    return (!m_timer.empty() && m_timer.front()->m_next_timeout <= _now()) || !m_is_running;
                });
            }
            m_timer.pop_front();

            // decrease remaining repetitions
            if (!next_timer->_is_infinite()) { --next_timer->m_times_left; }
        }

        // fire the timer's callback
        next_timer->m_callback();
    }
}

void TheTimerPool::_schedule(TimerPtr timer)
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (m_timer.empty()) {
        m_timer.emplace_front(std::move(timer));
        return;
    }
    for (auto prev = m_timer.before_begin(), it = m_timer.begin(), end = m_timer.end(); it != end; prev = ++it) {
        if ((*it)->m_next_timeout <= timer->m_next_timeout) {
            m_timer.emplace_after(prev, std::move(timer));
            break;
        }
    }
}

void TheTimerPool::_unschedule(const valid_ptr<Timer*> timer)
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (m_timer.empty()) { return; }
    for (auto prev = m_timer.before_begin(), it = m_timer.begin(), end = m_timer.end(); it != end; prev = ++it) {
        if ((*it).get() == raw_pointer(timer)) {
            m_timer.erase_after(prev);
            break;
        }
    }
}

// ================================================================================================================== //

Timer::~Timer() = default;

void Timer::start(const timepoint_t timeout)
{
    TheTimerPool& pool = TheTimerPool::get();
    {
        NOTF_GUARD(std::lock_guard(pool.m_mutex));

        // unschedule if active
        if (_is_active()) { pool._unschedule(this); }

        // this is a one-off overload of `start`
        m_times_left = 1;

        // fire right away if the timeout is already in the past
        if (timeout <= TheTimerPool::_now()) {
            m_next_timeout = timepoint_t();
            m_callback();
            return;
        }

        // or schedule the timer
        m_next_timeout = timeout;
        pool._schedule(shared_from_this());
    }
    pool.m_condition.notify_one();
}

void Timer::stop()
{
    TheTimerPool& pool = TheTimerPool::get();
    {
        NOTF_GUARD(std::lock_guard(pool.m_mutex));

        if (!_is_active()) { return; }

        // unschedule from the manager
        pool._unschedule(this);
    }
    pool.m_condition.notify_one();
}

// ================================================================================================================== //

OneShotTimer::~OneShotTimer() = default;

// ================================================================================================================== //

IntervalTimer::~IntervalTimer() = default;

void IntervalTimer::start(const duration_t interval, const size_t repetitions)
{
    if (repetitions == 0) {
        return; // easy
    }

    TheTimerPool& pool = TheTimerPool::get();
    {
        NOTF_GUARD(std::lock_guard(pool.m_mutex));

        // unschedule if active
        if (_is_active()) { pool._unschedule(this); }

        // fire right away if the timeout is already in the past
        m_interval = interval;
        if (interval == duration_t(0)) {
            m_next_timeout = timepoint_t();
            m_times_left = (repetitions == _infinity()) ? 1 : repetitions;
            for (; m_times_left != 0; --m_times_left) {
                m_callback();
            }
            return;
        }

        // or schedule the timer
        m_times_left = repetitions;
        m_next_timeout = TheTimerPool::_now() + interval;
        pool._schedule(shared_from_this());
    }
    pool.m_condition.notify_one();
}

// ================================================================================================================== //

VariableTimer::~VariableTimer() = default;

void VariableTimer::start(IntervalFunction function, const size_t repetitions)
{
    if (repetitions == 0) {
        return; // easy
    }

    TheTimerPool& pool = TheTimerPool::get();
    {
        NOTF_GUARD(std::lock_guard(pool.m_mutex));

        // unschedule if active
        if (_is_active()) { pool._unschedule(this); }

        // schedule the timer
        m_times_left = repetitions;
        m_function = std::move(function);
        m_next_timeout = TheTimerPool::_now() + m_function();
        pool._schedule(shared_from_this());
    }
    pool.m_condition.notify_one();
}

NOTF_CLOSE_NAMESPACE
