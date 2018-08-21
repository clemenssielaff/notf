#pragma once

/// The base code for this class is copied from:
///     https://github.com/HowardHinnant/upgrade_mutex
/// It is licensed with the following text:
/// ---------------------------------------------------------------------------
/// This software is in the public domain.  The only restriction on its use is
/// that no one can remove it from the public domain by claiming ownership of it,
/// including the original authors.
///
/// There is no warranty of correctness on the software contained herein.  Use
/// at your own risk.
/// ---------------------------------------------------------------------------

#include <chrono>
#include <climits>
#include <mutex>
#include <shared_mutex>
#include <system_error>

#include "common/meta.hpp"

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

///
/// See https://stackoverflow.com/a/13445989 for a comprehensive introduction from the master himself.
///
class UpgradeMutex {

    // types  ------------------------------------------------------------------------------------------------------- //
private:
    enum State : uint {
        UNLOCKED = 0,
        WRITE_ENTERED = 1U << (sizeof(uint) * CHAR_BIT - 1),
        UPGRADABLE_ENTERED = WRITE_ENTERED >> 1,
        N_READERS = ~(WRITE_ENTERED | UPGRADABLE_ENTERED),
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(UpgradeMutex);

    /// Default constructor.
    UpgradeMutex() = default;

    /// Destructor.
    ~UpgradeMutex() = default;

    // exclusive ownership ----------------------------------------------------

    void lock()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) {
            m_gate1.wait(lock);
        }
        m_state |= WRITE_ENTERED;
        while (m_state & N_READERS) {
            m_gate2.wait(lock);
        }
    }

    bool try_lock()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (m_state == UNLOCKED) {
            m_state = WRITE_ENTERED;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_lock_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) {
            while (true) {
                const std::cv_status status = m_gate1.wait_until(lock, abs_time);
                if ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) == 0) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        m_state |= WRITE_ENTERED;
        if (m_state & N_READERS) {
            while (true) {
                const std::cv_status status = m_gate2.wait_until(lock, abs_time);
                if ((m_state & N_READERS) == 0) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    m_state &= ~WRITE_ENTERED;
                    return false;
                }
            }
        }
        return true;
    }

    void unlock()
    {
        {
            NOTF_GUARD(std::lock_guard(m_mutex));
            m_state = UNLOCKED;
        }
        m_gate1.notify_all();
    }

    // shared ownership -------------------------------------------------------

    void lock_shared()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while ((m_state & WRITE_ENTERED) || (m_state & N_READERS) == N_READERS) {
            m_gate1.wait(lock);
        }
        const uint num_readers = (m_state & N_READERS) + 1;
        m_state &= ~N_READERS;
        m_state |= num_readers;
    }

    bool try_lock_shared()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        uint num_readers = m_state & N_READERS;
        if (!(m_state & WRITE_ENTERED) && num_readers != N_READERS) {
            ++num_readers;
            m_state &= ~N_READERS;
            m_state |= num_readers;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_lock_shared_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if ((m_state & WRITE_ENTERED) || (m_state & N_READERS) == N_READERS) {
            while (true) {
                const std::cv_status status = m_gate1.wait_until(lock, abs_time);
                if ((m_state & WRITE_ENTERED) == 0 && (m_state & N_READERS) < N_READERS) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        const uint num_readers = (m_state & N_READERS) + 1;
        m_state &= ~N_READERS;
        m_state |= num_readers;
        return true;
    }

    void unlock_shared()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        const uint num_readers = (m_state & N_READERS) - 1;
        m_state &= ~N_READERS;
        m_state |= num_readers;
        if (m_state & WRITE_ENTERED) {
            if (num_readers == 0) {
                m_gate2.notify_one();
            }
        }
        else {
            if (num_readers == N_READERS - 1) {
                m_gate1.notify_one();
            }
        }
    }

    // upgrade ownership ------------------------------------------------------

    void lock_upgrade()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) || (m_state & N_READERS) == N_READERS) {
            m_gate1.wait(lock);
        }
        const uint num_readers = (m_state & N_READERS) + 1;
        m_state &= ~N_READERS;
        m_state |= UPGRADABLE_ENTERED | num_readers;
    }

    bool try_lock_upgrade()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        uint num_readers = m_state & N_READERS;
        if (!(m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) && num_readers != N_READERS) {
            ++num_readers;
            m_state &= ~N_READERS;
            m_state |= UPGRADABLE_ENTERED | num_readers;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_lock_upgrade_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_lock_upgrade_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_lock_upgrade_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) || (m_state & N_READERS) == N_READERS) {
            while (true) {
                const std::cv_status status = m_gate1.wait_until(lock, abs_time);
                if ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) == 0 && (m_state & N_READERS) < N_READERS) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        const uint num_readers = (m_state & N_READERS) + 1;
        m_state &= ~N_READERS;
        m_state |= UPGRADABLE_ENTERED | num_readers;
        return true;
    }

    void unlock_upgrade()
    {
        {
            NOTF_GUARD(std::lock_guard(m_mutex));
            const uint num_readers = (m_state & N_READERS) - 1;
            m_state &= ~(UPGRADABLE_ENTERED | N_READERS);
            m_state |= num_readers;
        }
        m_gate1.notify_all();
    }

    // shared <-> exclusive ---------------------------------------------------

    bool try_unlock_shared_and_lock()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (m_state == 1) {
            m_state = WRITE_ENTERED;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_unlock_shared_and_lock_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_unlock_shared_and_lock_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_unlock_shared_and_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_state != 1) {
            while (true) {
                const std::cv_status status = m_gate2.wait_until(lock, abs_time);
                if (m_state == 1) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        m_state = WRITE_ENTERED;
        return true;
    }

    void unlock_and_lock_shared()
    {
        {
            NOTF_GUARD(std::lock_guard(m_mutex));
            m_state = 1;
        }
        m_gate1.notify_all();
    }

    // shared <-> upgrade -----------------------------------------------------

    bool try_unlock_shared_and_lock_upgrade()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (!(m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED))) {
            m_state |= UPGRADABLE_ENTERED;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_unlock_shared_and_lock_upgrade_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_unlock_shared_and_lock_upgrade_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_unlock_shared_and_lock_upgrade_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) != 0) {
            while (true) {
                const std::cv_status status = m_gate2.wait_until(lock, abs_time);
                if ((m_state & (WRITE_ENTERED | UPGRADABLE_ENTERED)) == 0) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        m_state |= UPGRADABLE_ENTERED;
        return true;
    }

    void unlock_upgrade_and_lock_shared()
    {
        {
            NOTF_GUARD(std::lock_guard(m_mutex));
            m_state &= ~UPGRADABLE_ENTERED;
        }
        m_gate1.notify_all();
    }

    // upgrade <-> exclusive --------------------------------------------------

    void unlock_upgrade_and_lock()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        const uint num_readers = (m_state & N_READERS) - 1;
        m_state &= ~(UPGRADABLE_ENTERED | N_READERS);
        m_state |= WRITE_ENTERED | num_readers;
        while (m_state & N_READERS) {
            m_gate2.wait(lock);
        }
    }

    bool try_unlock_upgrade_and_lock()
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (m_state == (UPGRADABLE_ENTERED | 1)) {
            m_state = WRITE_ENTERED;
            return true;
        }
        return false;
    }

    template<class Rep, class Period>
    bool try_unlock_upgrade_and_lock_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_unlock_upgrade_and_lock_until(std::chrono::steady_clock::now() + rel_time);
    }

    template<class Clock, class Duration>
    bool try_unlock_upgrade_and_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if ((m_state & N_READERS) != 1) {
            while (true) {
                const std::cv_status status = m_gate2.wait_until(lock, abs_time);
                if ((m_state & N_READERS) == 1) {
                    break;
                }
                if (status == std::cv_status::timeout) {
                    return false;
                }
            }
        }
        m_state = WRITE_ENTERED;
        return true;
    }

    void unlock_and_lock_upgrade()
    {
        {
            NOTF_GUARD(std::lock_guard(m_mutex));
            m_state = UPGRADABLE_ENTERED | 1;
        }
        m_gate1.notify_all();
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    std::mutex m_mutex;

    std::condition_variable m_gate1;

    std::condition_variable m_gate2;

    uint m_state = UNLOCKED;
};

// ================================================================================================================== //

class UpgradeLock {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(UpgradeLock);

    /// Default Constructor.
    UpgradeLock() noexcept = default;

    /// Destructor.
    ~UpgradeLock()
    {
        if (m_is_owning) {
            m_mutex->unlock_upgrade();
        }
    }

    /// Move constructor.
    /// @param other    Other UpgradeLock to move from.
    UpgradeLock(UpgradeLock&& other) noexcept : m_mutex(other.m_mutex), m_is_owning(other.m_is_owning)
    {
        other.m_mutex = nullptr;
        other.m_is_owning = false;
    }

    /// Move assignment.
    /// @param other    Other UpgradeLock to move from.
    UpgradeLock& operator=(UpgradeLock&& other)
    {
        if (m_is_owning) {
            m_mutex->unlock_upgrade();
        }
        m_mutex = other.m_mutex;
        m_is_owning = other.m_is_owning;
        other.m_mutex = nullptr;
        other.m_is_owning = false;
        return *this;
    }

    /// Value Constructor.
    /// Locks the given mutex with upgradable ownership.
    /// @param mutex    UpgardeMutex to lock.
    explicit UpgradeLock(UpgradeMutex& mutex) : m_mutex(&mutex), m_is_owning(true) { m_mutex->lock_upgrade(); }

    /// Value Constructor.
    /// Does not acquire ownership of the mutex.
    /// @param mutex    UpgardeMutex to lock later.
    UpgradeLock(UpgradeMutex& mutex, std::defer_lock_t) noexcept : m_mutex(&mutex), m_is_owning(false) {}

    /// Value Constructor.
    /// Tries to lock the given mutex with upgradable ownership. Use `owns_lock` to check if the lock succeeded.
    /// @param mutex    UpgardeMutex to lock.
    UpgradeLock(UpgradeMutex& mutex, std::try_to_lock_t) : m_mutex(&mutex), m_is_owning(mutex.try_lock_upgrade()) {}

    /// Value Constructor.
    /// Assumes that the calling thread has already obtained mutex ownership and only manages it.
    /// @param mutex    UpgardeMutex to lock.
    UpgradeLock(UpgradeMutex& mutex, std::adopt_lock_t) noexcept : m_mutex(&mutex), m_is_owning(true) {}

    /// Value Constructor.
    /// Tries to lock until the given time point.
    /// @param mutex        UpgardeMutex to lock.
    /// @param time_point   Time point after which the lock gives up to acquire the mutex.
    template<class Clock, class Duration>
    UpgradeLock(UpgradeMutex& m, const std::chrono::time_point<Clock, Duration>& time_point)
        : m_mutex(&m), m_is_owning(m.try_lock_upgrade_until(time_point))
    {}

    /// Value Constructor.
    /// Tries to lock for a given duration.
    /// @param mutex        UpgardeMutex to lock.
    /// @param time_point   Wait duration until the lock gives up to acquire the mutex.
    template<class Rep, class Period>
    UpgradeLock(UpgradeMutex& m, const std::chrono::duration<Rep, Period>& rel_time)
        : m_mutex(&m), m_is_owning(m.try_lock_upgrade_for(rel_time))
    {}

    // shared <-> upgrade -----------------------------------------------------

    UpgradeLock(std::shared_lock<UpgradeMutex>&& lock, std::try_to_lock_t) : m_mutex(nullptr), m_is_owning(false)
    {
        if (lock.owns_lock()) {
            if (lock.mutex()->try_unlock_shared_and_lock_upgrade()) {
                m_mutex = lock.release();
                m_is_owning = true;
            }
        }
        else {
            m_mutex = lock.release();
        }
    }

    template<class Clock, class Duration>
    UpgradeLock(std::shared_lock<UpgradeMutex>&& lock, const std::chrono::time_point<Clock, Duration>& time_point)
        : m_mutex(nullptr), m_is_owning(false)
    {
        if (lock.owns_lock()) {
            if (lock.mutex()->try_unlock_shared_and_lock_upgrade_until(time_point)) {
                m_mutex = lock.release();
                m_is_owning = true;
            }
        }
        else {
            m_mutex = lock.release();
        }
    }

    template<class Rep, class Period>
    UpgradeLock(std::shared_lock<UpgradeMutex>&& lock, const std::chrono::duration<Rep, Period>& duration)
        : m_mutex(nullptr), m_is_owning(false)
    {
        if (lock.owns_lock()) {
            if (lock.mutex()->try_unlock_shared_and_lock_upgrade_for(duration)) {
                m_mutex = lock.release();
                m_is_owning = true;
            }
        }
        else
            m_mutex = lock.release();
    }

    explicit operator std::shared_lock<UpgradeMutex>() &&
    {
        if (m_is_owning) {
            m_mutex->unlock_upgrade_and_lock_shared();
            return std::shared_lock<UpgradeMutex>(*release(), std::adopt_lock);
        }
        if (m_mutex != nullptr)
            return std::shared_lock<UpgradeMutex>(*release(), std::defer_lock);
        return std::shared_lock<UpgradeMutex>{};
    }

    // exclusive <-> upgrade --------------------------------------------------

    explicit UpgradeLock(std::unique_lock<UpgradeMutex>&& lock) : m_mutex(lock.mutex()), m_is_owning(lock.owns_lock())
    {
        if (m_is_owning) {
            m_mutex->unlock_and_lock_upgrade();
        }
        lock.release();
    }

    explicit operator std::unique_lock<UpgradeMutex>() &&
    {
        if (m_is_owning) {
            m_mutex->unlock_upgrade_and_lock();
            return std::unique_lock<UpgradeMutex>(*release(), std::adopt_lock);
        }
        if (m_mutex != nullptr)
            return std::unique_lock<UpgradeMutex>(*release(), std::defer_lock);
        return std::unique_lock<UpgradeMutex>{};
    }

    // upgrade ----------------------------------------------------------------

    void lock()
    {
        if (m_mutex == nullptr) {
            throw std::system_error(std::error_code(EPERM, std::system_category()),
                                    "UpgradeLock::lock: references null mutex");
        }
        if (m_is_owning) {
            throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                    "UpgradeLock::lock: already locked");
        }
        m_mutex->lock_upgrade();
        m_is_owning = true;
    }

    bool try_lock()
    {
        if (m_mutex == nullptr) {
            throw std::system_error(std::error_code(EPERM, std::system_category()),
                                    "UpgradeLock::try_lock: references null mutex");
        }
        if (m_is_owning) {
            throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                    "UpgradeLock::try_lock: already locked");
        }
        m_is_owning = m_mutex->try_lock_upgrade();
        return m_is_owning;
    }

    template<class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& duration)
    {
        return try_lock_until(std::chrono::steady_clock::now() + duration);
    }

    template<class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& time_point)
    {
        if (m_mutex == nullptr) {
            throw std::system_error(std::error_code(EPERM, std::system_category()),
                                    "UpgradeLock::try_lock_until: references null mutex");
        }
        if (m_is_owning) {
            throw std::system_error(std::error_code(EDEADLK, std::system_category()),
                                    "UpgradeLock::try_lock_until: already locked");
        }
        m_is_owning = m_mutex->try_lock_upgrade_until(time_point);
        return m_is_owning;
    }

    void unlock()
    {
        if (!m_is_owning) {
            throw std::system_error(std::error_code(EPERM, std::system_category()), "UpgradeLock::unlock: not locked");
        }
        m_mutex->unlock_upgrade();
        m_is_owning = false;
    }

    void swap(UpgradeLock& other)
    {
        std::swap(m_mutex, other.m_mutex);
        std::swap(m_is_owning, other.m_is_owning);
    }

    UpgradeMutex* release()
    {
        UpgradeMutex* r = m_mutex;
        m_mutex = nullptr;
        m_is_owning = false;
        return r;
    }

    bool owns_lock() const { return m_is_owning; }

    explicit operator bool() const { return m_is_owning; }

    UpgradeMutex* mutex() const { return m_mutex; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    UpgradeMutex* m_mutex = nullptr;

    bool m_is_owning = false;

    struct __nat { // this might be compiler black magic ... just ignore it
        int _;
    };
};

inline void swap(UpgradeLock& x, UpgradeLock& y) { x.swap(y); }

NOTF_CLOSE_NAMESPACE
