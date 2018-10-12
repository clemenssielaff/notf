#pragma once

#include <thread>

#include "notf/meta/exception.hpp"
#include "notf/meta/macros.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Simple thread guard to ensure that a thread is always joined back.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.3
class ThreadGuard {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(ThreadGuard);

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

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Thread to guard.
    std::thread& m_thread;
};

// ================================================================================================================== //

/// Wraps and owns a thread that is automatically joined when destructed.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.6
class ScopedThread {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(ScopedThread);

    /// Default constructor.
    ScopedThread() = default;

    /// Constructor
    /// @param thread       Thread to guard.
    /// @throws logic_error If the thread is not joinable.
    explicit ScopedThread(std::thread thread) : m_thread(std::move(thread))
    {
        if (!m_thread.joinable()) {
            NOTF_THROW(logic_error, "Cannot guard non-joinable thread");
        }
    }

    /// Move Constructor.
    /// @param other    ScopedThread to move from.
    ScopedThread(ScopedThread&& other) : ScopedThread(std::move(other.m_thread)) { other.m_thread = {}; }

    /// Move Assignment.
    /// Blocks until the the current thread is joined (if it is joinable).
    /// @param other    ScopedThread to move from.
    ScopedThread& operator=(ScopedThread&& other)
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
        m_thread = std::move(other.m_thread);
        other.m_thread = {};
        return *this;
    }

    /// Destructor.
    /// Blocks until the thread has joined.
    ~ScopedThread()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Thread to guard.
    std::thread m_thread;
};

NOTF_CLOSE_NAMESPACE
