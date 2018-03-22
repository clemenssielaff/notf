#pragma once

#include <mutex>
#include <thread>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

#ifdef NOTF_DEBUG

/// In debug mode, the notf::Mutex can be asked to check whether it is locked by the calling thread.
/// From https://stackoverflow.com/a/30109512/3444217
class Mutex : public std::mutex {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Locks the mutex.
    void lock()
    {
        std::mutex::lock();
        m_holder = std::this_thread::get_id();
    }

    /// Unlocks the mutex.
    void unlock()
    {
        m_holder = std::thread::id();
        std::mutex::unlock();
    }

    /// Checks if the mutex is locked by the thread calling this method.
    bool is_locked_by_this_thread() const { return m_holder == std::this_thread::get_id(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Id of the thread currently holding this mutex.
    std::thread::id m_holder;
};

#else // !NOTF_DEBUG

/// In release mode, the notf::Mutex is just a std::mutex.
using Mutex = std::mutex;

#endif // NOTF_DEBUG

NOTF_CLOSE_NAMESPACE
