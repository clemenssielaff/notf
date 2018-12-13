#include "notf/common/thread_pool.hpp"

NOTF_OPEN_NAMESPACE

// thread pool ====================================================================================================== //

ThreadPool::ThreadPool(const size_t thread_count) {
    m_workers.reserve(thread_count);

    // create the worker threads
    for (size_t i = 0; i < thread_count; ++i) {
        m_workers.emplace_back(Thread::Kind::WORKER);
        m_workers.back().run([this] {
            // worker function
            while (true) {
                Delegate<void()> task;
                { // grab new tasks while there are any
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_condition_variable.wait(lock, [this] { return !m_tasks.empty() || m_is_finished; });
                    if (m_is_finished && m_tasks.empty()) { return; }
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                // execute the new task
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    { // notify all workers that the pool is finished
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_is_finished = true;
    }
    m_condition_variable.notify_all();

    // join all workers and finish
    m_workers.clear();
}

NOTF_CLOSE_NAMESPACE
