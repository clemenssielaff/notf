#include "notf/app/event/scheduler.hpp"

#include <iostream>

#include "notf/app/event/window_resize_event.hpp"

NOTF_OPEN_NAMESPACE

// scheduler ======================================================================================================== //

Scheduler::Scheduler() : m_thread(Thread(Thread::Kind::EVENT)) {
    m_thread.run([&]() {
        while (true) {
            { // wait until the next event is ready
                std::unique_lock<Mutex> lock(m_mutex);
                if (m_events.empty() && m_is_running) {
                    m_condition.wait(lock, [&] { return !m_events.empty() || !m_is_running; });
                }

                // stop condition
                if (!m_is_running) { return; }
            }

            // handle event
            AnyEvent next_event = std::move(m_events.front());
            m_events.pop_front();
        }
    });
}

Scheduler::~Scheduler() { stop(); }

void Scheduler::schedule(AnyEvent&& event) {
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        m_events.emplace_back(std::move(event));
    }
    m_condition.notify_one();
    std::cout << "Scheduler notified" << std::endl;
}

void Scheduler::stop() {
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (!m_is_running) { return; }
        m_is_running = false;
        m_events.clear();
    }
    m_condition.notify_one();
    m_thread = {}; // blocks
}

NOTF_CLOSE_NAMESPACE
