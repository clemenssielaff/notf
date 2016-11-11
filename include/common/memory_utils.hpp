#pragma once

#ifdef _DEBUG
#include <assert.h>
#include <memory>
#endif

namespace notf {

#ifdef _DEBUG

/**
 * Raw pointer to an object managed by a shared pointer, debug (checked) version.
 */
template <typename T>
struct raw_ptr {

    std::weak_ptr<T> m_ptr;

    explicit raw_ptr(std::shared_ptr<T> ptr)
        : m_ptr(ptr)
    {
    }

    T* get() const
    {
        std::shared_ptr<T> locked = m_ptr.lock();
        assert(locked);
        return locked.get();
    }
};

#else

/**
 * Raw pointer to an object managed by a shared pointer, release (unchecked) version.
 */
template <typename T>
struct raw_ptr {

    T* m_ptr;

    explicit raw_ptr(std::shared_ptr<T> ptr)
        : m_ptr(ptr.get())
    {
    }

    T* get() const { return m_ptr; }
};

#endif

} // namespace notf
