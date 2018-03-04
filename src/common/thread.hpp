#pragma once

#include <thread>

#include "common/exception.hpp"
#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Simple thread guard to ensure that a thread is always joined back.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.3
class ThreadGuard {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(ThreadGuard)

    /// Constructor
    /// @param thread   Thread to guard.
    explicit ThreadGuard(std::thread& thread) : m_thread(thread) {}

    /// Destructor.
    /// Blocks until the thread has joined (does not block if the thread was already joined).
    ~ThreadGuard()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Thread to guard.
    std::thread& m_thread;
};

//====================================================================================================================//

/// Wraps and owns a thread that is automatically joined when destructed.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.6
class ScopedThread {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(ScopedThread)

    /// Constructor
    /// @param thread       Thread to guard.
    /// @throws logic_error If the thread is not joinable.
    explicit ScopedThread(std::thread thread) : m_thread(std::move(thread))
    {
        if (!m_thread.joinable()) {
            notf_throw(logic_error, "Cannot guard non-joinable thread");
        }
    }

    /// Move Constructor.
    /// @param other    ScopedThread to move from.
    ScopedThread(ScopedThread&& other) : ScopedThread(std::move(other.m_thread)) { other.m_thread = {}; }

    /// Destructor.
    /// Blocks until the thread has joined.
    ~ScopedThread()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Thread to guard.
    std::thread m_thread;
};

//====================================================================================================================//

/* TODO: thread registry
ideally, we would be able to name a thread. But instead of creating a data structure that boundles a std::thread with a
std::string, there should be a common datastructure (perhaps a std::unordered_map<std::thread::id, std::string>) that
associates a thread ID with a name. Names are const, once defined so there should be no data races there, as long as I
don't dynamically create new threads.
 */

/* TODO: debug context
a global debug context, that keeps track of various debug stats like number of times a mutex was contested etc.?
 */
NOTF_CLOSE_NAMESPACE
