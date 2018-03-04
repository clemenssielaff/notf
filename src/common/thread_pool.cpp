#include "common/thread_pool.hpp"

namespace notf {

//====================================================================================================================//

thread_pool_finished::~thread_pool_finished() {}

//====================================================================================================================//

ThreadPool::ThreadPool(const size_t thread_count)
{
    m_workers.reserve(thread_count);

    // create the worker threads
    for (size_t i = 0; i < thread_count; ++i) {
        m_workers.emplace_back([&] {

            // worker function
            while (true) {
                std::function<void()> task;
                { // grab new tasks while there are any
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    this->m_condition_variable.wait(lock, [&] { return !m_tasks.empty() || is_finished; });
                    if (is_finished && m_tasks.empty()) {
                        return;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                // execute the new task
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    { // notify all workers that we're finished
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        is_finished = true;
    }
    m_condition_variable.notify_all();

    // join all workers and finish
    for (std::thread& worker : m_workers) {
        worker.join();
    }
}

} // namespace notf