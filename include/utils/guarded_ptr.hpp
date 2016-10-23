#pragma once

/*
 * @brief Pointer that acts like a raw pointer but is checked against invalidation in debug mode.
 */

#include <assert.h>
#include <memory>

#ifdef _DEBUG

template <typename T>
class GuardedPtr {

public: // methods
    /// @brief Default Constructor.
    GuardedPtr() = default;

    /// @brief Value Constructor.
    /// @param shared_ptr   Owning pointer to the referenced object.
    GuardedPtr(std::shared_ptr<T> shared_ptr)
        : m_ptr(shared_ptr)
    {
    }

    /// @brief Returns the referenced object.
    T* get() const
    {
        assert(!m_ptr.expired());
        return m_ptr.lock().get();
    }

private: // fields
    /// @brief The referenced object.
    std::weak_ptr<T> m_ptr;
};

#else

template <typename T>
class GuardedPtr {

public: // methods
    /// @brief Default Constructor.
    GuardedPtr() = default;

    /// @brief Value Constructor.
    /// @param shared_ptr   Owning pointer to the referenced object.
    GuardedPtr(std::shared_ptr<T> shared_ptr)
        : m_ptr(shared_ptr.get())
    {
    }

    /// @brief Returns the referenced object.
    T* get() const { return m_ptr; }

private: // fields
    /// @brief The referenced object.
    T* m_ptr;
};

#endif
