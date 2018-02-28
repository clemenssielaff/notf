#pragma once

#include <thread>

#include "common/meta.hpp"

namespace notf {

//====================================================================================================================//

/// Simple thread guard to ensure that a thread is always joined back.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.3
class ThreadGuard {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NO_COPY_AND_ASSIGN(ThreadGuard)

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

} // namespace notf
