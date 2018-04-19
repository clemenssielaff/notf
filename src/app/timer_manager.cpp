#include "app/timer_manager.hpp"

#include "app/application.hpp"

NOTF_OPEN_NAMESPACE

TimerManager::TimerManager() { m_thread = ScopedThread(std::thread(&TimerManager::_run, this)); }

TimerManager::~TimerManager()
{
    {
        std::unique_lock<std::mutex> lock_guard(m_mutex);
        m_is_running = false;
    }
    m_condition.notify_one();
    m_thread = {}; // blocks until the thread has joined
}

void TimerManager::_run()
{
    TimerPtr next_timer = Timer::create([] {});
    while (1) {
        {
            std::unique_lock<std::mutex> lock_guard(m_mutex);

            // re-schedule the last timer, if it is repeating AND if this is not the last reference to the timer
            // otherwise we could end up with timer that could never be stopped
            if (next_timer->is_repeating() && !next_timer.unique()) {
                next_timer->m_next_timeout += next_timer->m_interval;
                _schedule(std::move(next_timer));
            }

            next_timer.reset();
            while (!next_timer) {
                // stop the thread
                if (!m_is_running) {
                    return;
                }

                // pop the next timer if one is available
                if (!m_timer.empty() && m_timer.front()->m_next_timeout <= _now()) {
                    next_timer = std::move(m_timer.front());
                    break;
                }

                // wait until there is a timeout to wait for
                if (m_timer.empty()) {
                    m_condition.wait(lock_guard, [&] { return !m_timer.empty() || !m_is_running; });
                }

                // wait for the next timeout
                else {
                    m_condition.wait_until(lock_guard, m_timer.front()->m_next_timeout, [&] {
                        return (!m_timer.empty() && m_timer.front()->m_next_timeout <= _now()) || !m_is_running;
                    });
                }
            }
            m_timer.pop_front();
        }

        // fire the timer's callback
        next_timer->m_callback();
    }
}

void TimerManager::_schedule(TimerPtr timer)
{
    // TODO: it would be nice if we could use a condition_variable with our custom Mutex
    // NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
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
    // NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
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

//====================================================================================================================//

void Timer::start()
{
    duration_t interval;
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        interval = m_interval;
    }
    start(interval);
}

void Timer::start(const time_point_t timeout)
{
    TimerManager& manager = Application::instance().timer_manager();
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);

        // unschedule if active
        if (_is_active()) {
            std::unique_lock<std::mutex> lock_guard(manager.m_mutex);
            manager._unschedule(this);
        }

        // this is a one-off overload of `start`
        m_interval = duration_t(0);

        // fire right away if the timeout is already in the past
        if (timeout <= TimerManager::_now()) {
            m_next_timeout = time_point_t();
            return m_callback();
        }

        // or schedule the timer
        m_next_timeout = timeout;
        {
            std::unique_lock<std::mutex> lock_guard(manager.m_mutex);
            manager._schedule(shared_from_this());
        }
        manager.m_condition.notify_one();
    }
}

void Timer::start(const duration_t interval, const bool is_repeating)
{
    TimerManager& manager = Application::instance().timer_manager();
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);

        // unschedule if active
        if (_is_active()) {
            std::unique_lock<std::mutex> lock_guard(manager.m_mutex);
            manager._unschedule(this);
        }

        // an interval of zero is equivalent to `is_repeating` set to false
        m_interval = is_repeating ? interval : duration_t(0);

        // fire right away if the timeout is already in the past
        if (interval == duration_t(0)) {
            m_next_timeout = time_point_t();
            return m_callback();
        }

        // or schedule the timer
        m_next_timeout = TimerManager::_now() + interval;
        {
            std::unique_lock<std::mutex> lock_guard(manager.m_mutex);
            manager._schedule(shared_from_this());
        }
        manager.m_condition.notify_one();
    }
}

void Timer::stop()
{
    TimerManager& manager = Application::instance().timer_manager();
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);

        if (!_is_active()) {
            return;
        }

        { // unschedule from the manager
            std::unique_lock<std::mutex> lock_guard(manager.m_mutex);
            m_next_timeout = time_point_t();
            manager._unschedule(this);
        }
        manager.m_condition.notify_one();
    }
}

NOTF_CLOSE_NAMESPACE
