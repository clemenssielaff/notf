#pragma once

#include <mutex>

#include "notf/common/thread.hpp"

NOTF_OPEN_NAMESPACE

// mutex ============================================================================================================ //

/// Mutex that can be asked to check whether it is locked by the calling thread.
/// From https://stackoverflow.com/a/30109512
class Mutex : public std::mutex {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Locks the mutex.
    void lock() {
        std::mutex::lock();
        m_holder = to_number(this_thread::get_id());
    }

    /// Tries to lock the mutex.
    /// @returns    True, iff the lock acquisition was successfull.
    bool try_lock() {
        if (std::mutex::try_lock()) {
            m_holder = to_number(this_thread::get_id());
            return true;
        } else {
            return false;
        }
    }

    /// Unlocks the mutex.
    void unlock() {
        m_holder = to_number(std::thread::id());
        std::mutex::unlock();
    }

    /// Checks if the mutex is locked by the thread calling this method.
    bool is_locked_by_this_thread() const { return m_holder == to_number(this_thread::get_id()); }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Id of the thread currently holding this mutex.
    std::atomic<Thread::NumericId> m_holder = to_number(std::thread::id());
};

// recursive mutex ================================================================================================== //

/// RecursiveMutex that can check whether it is locked by the calling thread and return the
class RecursiveMutex : public std::recursive_mutex {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Locks the mutex.
    void lock() {
        std::recursive_mutex::lock();
        m_holder = to_number(this_thread::get_id());
        ++m_counter;
    }

    /// Tries to lock the mutex.
    /// @returns    True, iff the lock acquisition was successfull.
    bool try_lock() {
        if (std::recursive_mutex::try_lock()) {
            m_holder = to_number(this_thread::get_id());
            ++m_counter;
            return true;
        } else {
            return false;
        }
    }

    /// Unlocks the mutex.
    void unlock() {
        NOTF_ASSERT(m_counter > 0);
        if (--m_counter == 0) { m_holder = to_number(std::thread::id()); }
        std::recursive_mutex::unlock();
    }

    /// Checks if the mutex is locked by the thread calling this method.
    bool is_locked_by_this_thread() const { return m_holder == to_number(this_thread::get_id()); }

    /// Number of times this mutex is locked by the calling thread.
    /// Calling this method from a thread that has not locked this mutex will work, but the result will be meaningless.
    size_t get_recursion_counter() const { return m_counter; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Id of the thread currently holding this mutex.
    std::atomic<Thread::NumericId> m_holder = to_number(std::thread::id());

    /// How often the mutex has been locked.
    std::atomic_size_t m_counter = 0;
};

NOTF_CLOSE_NAMESPACE
