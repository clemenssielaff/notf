#pragma once

#include <deque>
#include <future>
#include <vector>

#include "notf/meta/exception.hpp"

#include "notf/common/thread.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

///
/// Modelled closely after:
///     https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
/// and
///     http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
///
class ThreadPool {

    /// Thrown when you enqueue a new thread in a ThreadPool that has already finished.
    NOTF_EXCEPTION_TYPE(FinishedError);

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(ThreadPool);

    /// Constructor.
    /// @param thread_count     Number of threads in the pool.
    ThreadPool(size_t thread_count = std::max(std::thread::hardware_concurrency(), 2u) - 1u);

    /// Destructor.
    /// Finishes all outstanding tasks before returning.
    ~ThreadPool();

    /// Enqueues a new task without return value.
    /// @param function     Function returning void.
    /// @param args         Arguments forwareded to the function when the tasks is executed.
    /// @throws             thread_pool_finished When the thread pool has already finished.
    template<class Function, class... Args, class return_t = std::invoke_result_t<Function(Args...)>>
    std::enable_if_t<std::is_same_v<void, return_t>, void> enqueue(Function&& function, Args&&... args)
    {
        { // enqueue the new task, unless the pool has already finished
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (NOTF_UNLIKELY(m_is_finished)) {
                NOTF_THROW(thread_pool_finished_error, "Cannot enqueue a new task into an already finished ThreadPool");
            }
            m_tasks.emplace_back(
                [function{std::forward<Function>(function)}, &args...] { function(std::forward<Args>(args)...); });
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
    template<class Function, class... Args, class return_t = std::invoke_result_t<Function(Args...)>>
    NOTF_NODISCARD std::enable_if_t<!std::is_same_v<void, return_t>, std::future<return_t>>
    enqueue(Function&& function, Args&&... args)
    {
        // create the new task
        auto task = std::make_shared<std::packaged_task<return_t()>>(
            [function{std::forward<Function>(function)}, &args...] { return function(std::forward<Args>(args)...); });
        std::future<return_t> result = task->get_future();

        { // enqueue the new task, unless the pool has already finished
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (NOTF_UNLIKELY(m_is_finished)) {
                NOTF_THROW(FinishedError, "Cannot enqueue a new task into an already finished ThreadPool");
            }
            m_tasks.emplace_back([task{std::move(task)}]() { (*task)(); });
        }
        m_condition_variable.notify_one();

        return result;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Worker threads.
    std::vector<Thread> m_workers;

    /// All outstanding tasks in the pool.
    std::deque<std::function<void()>> m_tasks;

    /// Mutex used to guard access to the queue and the `is_finished` flag.
    std::mutex m_queue_mutex;

    /// Condition variable to notify workers of new tasks in the queue.
    std::condition_variable m_condition_variable;

    /// Condition set to true when the ThreadPoool has finished.
    bool m_is_finished = false;
};

NOTF_CLOSE_NAMESPACE
