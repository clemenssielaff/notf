#include "app/timer_manager.hpp"

#include "app/application.hpp"

NOTF_OPEN_NAMESPACE

TimerManager::TimerManager() { m_thread = ScopedThread(std::thread(&TimerManager::_run, this)); }

TimerManager::~TimerManager()
{
    {
        NOTF_MUTEX_GUARD(m_mutex);
        m_is_running = false;
    }
    m_condition.notify_one();
    m_thread = {}; // blocks until the thread has joined
}

void TimerManager::_run()
{
    TimerPtr next_timer = IntervalTimer::create([] {});
    while (true) {
        {
            std::unique_lock<Mutex> lock(m_mutex);

            // re-schedule the last timer, if it is repeating AND if this is not the last reference to the timer
            // otherwise we could end up with timer that could never be stopped
            if ((next_timer->m_times_left > 0) && !next_timer.unique()) {
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
                if (!m_is_running) {
                    return;
                }
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
            if (!next_timer->_is_infinite()) {
                --next_timer->m_times_left;
            }
        }

        // fire the timer's callback
        next_timer->m_callback();
    }
}

void TimerManager::_schedule(TimerPtr timer)
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (m_timer.empty()) {
        m_timer.emplace_front(std::move(timer));
        return;
    }
    auto it = m_timer.begin();
    if ((*it)->m_next_timeout <= timer->m_next_timeout) {
        m_timer.emplace_front(std::move(timer));
    }
    else {
        auto prev = it;
        ++it;
        for (auto end = m_timer.end(); it != end; ++it) {
            if ((*it)->m_next_timeout <= timer->m_next_timeout) {
                break;
            }
            prev = it;
        }
        m_timer.emplace_after(prev, std::move(timer));
    }
}

void TimerManager::_unschedule(const valid_ptr<Timer*> timer)
{
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    if (m_timer.empty()) {
        return;
    }
    auto it = m_timer.begin();
    if ((*it).get() == timer) {
        m_timer.pop_front();
    }
    else {
        auto prev = it;
        ++it;
        for (auto end = m_timer.end(); it != end; ++it) {
            if ((*it).get() == timer) {
                m_timer.erase_after(prev);
                break;
            }
            prev = it;
        }
    }
}

// ================================================================================================================== //

Timer::~Timer() = default;

void Timer::start(const timepoint_t timeout)
{
    TimerManager& manager = Application::instance().get_timer_manager();
    {
        NOTF_MUTEX_GUARD(manager.m_mutex);

        // unschedule if active
        if (_is_active()) {
            manager._unschedule(this);
        }

        // this is a one-off overload of `start`
        m_times_left = 1;

        // fire right away if the timeout is already in the past
        if (timeout <= TimerManager::_now()) {
            m_next_timeout = timepoint_t();
            m_callback();
            return;
        }

        // or schedule the timer
        m_next_timeout = timeout;
        manager._schedule(shared_from_this());
    }
    manager.m_condition.notify_one();
}

void Timer::stop()
{
    TimerManager& manager = Application::instance().get_timer_manager();
    {
        NOTF_MUTEX_GUARD(manager.m_mutex);

        if (!_is_active()) {
            return;
        }

        // unschedule from the manager
        manager._unschedule(this);
    }
    manager.m_condition.notify_one();
}

// ================================================================================================================== //

OneShotTimer::~OneShotTimer() = default;

// ================================================================================================================== //

IntervalTimer::~IntervalTimer() = default;

void IntervalTimer::start(const duration_t interval, const size_t repetitions)
{
    if (repetitions == 0) {
        return; // what did you expect?
    }

    TimerManager& manager = Application::instance().get_timer_manager();
    {
        NOTF_MUTEX_GUARD(manager.m_mutex);

        // unschedule if active
        if (_is_active()) {
            manager._unschedule(this);
        }

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
        m_next_timeout = TimerManager::_now() + interval;
        manager._schedule(shared_from_this());
    }
    manager.m_condition.notify_one();
}

// ================================================================================================================== //

VariableTimer::~VariableTimer() = default;

void VariableTimer::start(IntervalFunction function, const size_t repetitions)
{
    if (repetitions == 0) {
        return; // what did you expect?
    }

    TimerManager& manager = Application::instance().get_timer_manager();
    {
        NOTF_MUTEX_GUARD(manager.m_mutex);

        // unschedule if active
        if (_is_active()) {
            manager._unschedule(this);
        }

        // schedule the timer
        m_times_left = repetitions;
        m_function = std::move(function);
        m_next_timeout = TimerManager::_now() + m_function();
        manager._schedule(shared_from_this());
    }
    manager.m_condition.notify_one();
}

NOTF_CLOSE_NAMESPACE
