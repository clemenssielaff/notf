#pragma once

#include <mutex>
#include <thread>

#include "common/assert.hpp"
#include "common/exception.hpp"
#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

#ifdef NOTF_DEBUG

/// In debug mode, the notf::Mutex can be asked to check whether it is locked by the calling thread.
/// From https://stackoverflow.com/a/30109512
class Mutex : public std::mutex {

    // methods ------------------------------------------------------------------------------------------------------ //
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

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Id of the thread currently holding this mutex.
    std::thread::id m_holder;
};

// ================================================================================================================== //

/// In debug mode, the notf::RecursiveMutex can be asked to check whether it is locked by the calling thread.
/// From https://stackoverflow.com/a/30109512
class RecursiveMutex : public std::recursive_mutex {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Locks the mutex.
    void lock()
    {
        std::recursive_mutex::lock();
        m_holder = std::this_thread::get_id();
        ++m_counter;
    }

    /// Unlocks the mutex.
    void unlock()
    {
        NOTF_ASSERT(m_counter > 0);
        if (--m_counter == 0) {
            m_holder = std::thread::id();
        }
        std::recursive_mutex::unlock();
    }

    /// Checks if the mutex is locked by the thread calling this method.
    bool is_locked_by_this_thread() const { return m_holder == std::this_thread::get_id(); }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Id of the thread currently holding this mutex.
    std::thread::id m_holder;

    /// How often the mutex has been locked.
    size_t m_counter = 0;
};

#else // !NOTF_DEBUG

/// In release mode, the notf mutexes are just std.
using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;

#endif // NOTF_DEBUG

// ================================================================================================================== //

/// Convenience macro to create a scoped lock_guard for a given mutex.
#define NOTF_MUTEX_GUARD(m) std::lock_guard<decltype(m)> _notf_mutex_lock_guard(m)

NOTF_CLOSE_NAMESPACE
