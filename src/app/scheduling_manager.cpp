#include "app/scheduling_manager.hpp"

#include "app/application.hpp"

NOTF_OPEN_NAMESPACE

SchedulingManager::~SchedulingManager()
{
    {
        std::unique_lock<Mutex> lock_guard(m_mutex);
        m_is_running = false;
    }
    m_condition.notify_one();
    m_thread = {}; // blocks until the thread has joined
}

void SchedulingManager::_run()
{
    while (1) {
        TimerPtr next_timer;
        {
            std::unique_lock<std::mutex> lock_guard(m_mutex);
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

        // re-schedule the timer, if it is repeating AND if this is not the last reference to the timer
        // otherwise we could end up with timer that could never be stopped
        if (next_timer->is_repeating() && !next_timer.unique()) {
            next_timer->m_next_timeout += next_timer->m_interval;
            auto it = m_timer.begin();
            for (auto end = m_timer.end(); it != end; ++it) {
                if ((*it)->m_next_timeout <= next_timer->m_next_timeout) {
                    break;
                }
            }
            m_timer.emplace(it, std::move(next_timer));
        }
    }
}

//====================================================================================================================//

void Timer::start(const duration_t interval, const bool is_repeating)
{
    if (is_active()) {
        stop();
    }
    SchedulingManager& manager = Application::instance().scheduling_manager();
    {
        std::unique_lock<Mutex> lock_guard(manager.m_mutex);
        m_next_timeout = SchedulingManager::_now() + interval;
        m_interval = is_repeating ? interval : duration_t(0);
        auto it = manager.m_timer.begin();
        for (auto end = manager.m_timer.end(); it != end; ++it) {
            if ((*it)->m_next_timeout <= m_next_timeout) {
                break;
            }
        }
        manager.m_timer.emplace(it, shared_from_this());
    }
    manager.m_condition.notify_one();
}

void Timer::stop()
{
    if (!is_active()) {
        return;
    }
    SchedulingManager& manager = Application::instance().scheduling_manager();
    {
        std::unique_lock<Mutex> lock_guard(manager.m_mutex);
        m_next_timeout = time_point_t();
        for (auto it = manager.m_timer.begin(), end = manager.m_timer.end(); it != end; ++it) {
            if ((*it).get() == this) {
                manager.m_timer.erase(it);
                break;
            }
        }
    }
    manager.m_condition.notify_one();
}

NOTF_CLOSE_NAMESPACE
