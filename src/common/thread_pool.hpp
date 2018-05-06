#pragma once

#include <deque>
#include <future>

#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thrown when you enqueue a new thread in a ThreadPool that has already finished.
NOTF_EXCEPTION_TYPE(thread_pool_finished);

//====================================================================================================================//

///
/// Modelled closely after:
///     https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
/// and
///     http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
///
class ThreadPool {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(ThreadPool);

    /// Constructor.
    /// @param thread_count     Number of threads in the pool.
    ThreadPool(const size_t thread_count = std::max(std::thread::hardware_concurrency(), 2u) - 1u);

    /// Destructor.
    /// Finishes all outstanding tasks before returning.
    ~ThreadPool();

    /// Enqueues a new task without return value.
    /// @param function     Function returning void.
    /// @param args         Arguments forwareded to the function when the tasks is executed.
    /// @throws             thread_pool_finished When the thread pool has already finished.
    template<typename FUNCTION, typename... Args, typename return_t = typename std::result_of<FUNCTION(Args...)>::type,
             typename = std::enable_if_t<(std::is_same<void, return_t>::value)>>
    void enqueue(FUNCTION&& function, Args&&... args)
    {
        { // enqueue the new task, unless the pool has already finished
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (is_finished) {
                notf_throw(thread_pool_finished, "Cannot enqueue a new task into an already finished ThreadPool");
            }
            m_tasks.emplace_back(
                [function{std::forward<FUNCTION>(function)}, &args...] { function(std::forward<Args>(args)...); });
        }
        m_condition_variable.notify_one();
    }

    /// Enqueues a new task with a return value.
    /// Note that this overload is more expensive than enqueuing a task without a return value, because it needs to wrap
    /// the function into a `packaged_task` with shared state for the returned future.
    /// Therefore we declare the return value NOTF_NODISCARD.
    /// If you want to ignore the return value, consider wrapping the callable in a simple lambda returning void before
    /// enqueuing it.
    /// @param function     Function.
    /// @param args         Arguments forwareded to the function when the tasks is executed.
    /// @returns            Future containing the result of the function once it finished execution.
    /// @throws             thread_pool_finished When the thread pool has already finished.
    template<typename FUNCTION, typename... Args, typename return_t = typename std::result_of<FUNCTION(Args...)>::type,
             typename = std::enable_if_t<!(std::is_same<void, return_t>::value)>>
    NOTF_NODISCARD std::future<return_t> enqueue(FUNCTION&& function, Args&&... args)
    {
        // create the new task
        auto task = std::make_shared<std::packaged_task<return_t()>>(
            [function{std::forward<FUNCTION>(function)}, &args...] { return function(std::forward<Args>(args)...); });
        std::future<return_t> result = task->get_future();

        { // enqueue the new task, unless the pool has already finished
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (is_finished) {
                notf_throw(thread_pool_finished, "Cannot enqueue a new task into an already finished ThreadPool");
            }
            m_tasks.emplace_back([task{std::move(task)}]() { (*task)(); });
        }
        m_condition_variable.notify_one();

        return result;
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Worker threads.
    std::vector<std::thread> m_workers;

    /// All outstanding tasks in the pool.
    std::deque<std::function<void()>> m_tasks;

    /// Mutex used to guard access to the queue and the `is_finished` flag.
    std::mutex m_queue_mutex;

    /// Condition variable to notify workers of new tasks in the queue.
    std::condition_variable m_condition_variable;

    /// Condition set to true when the ThreadPoool has finished.
    bool is_finished = false;
};

NOTF_CLOSE_NAMESPACE
