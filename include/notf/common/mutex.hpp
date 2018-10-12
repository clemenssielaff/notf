#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "./common.hpp"

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

/// In order to work with notf::Mutex in debug mode, we have to use condition_variable_any for condition variables.
using ConditionVariable = std::condition_variable_any;

#else // !NOTF_DEBUG

/// In release mode, the notf mutexes are just typedefs for standard types.
using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using ConditionVariable = std::condition_variable;

#endif // NOTF_DEBUG

NOTF_CLOSE_NAMESPACE
