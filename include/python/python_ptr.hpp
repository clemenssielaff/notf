#pragma once

#include <assert.h>
#include <memory>
#include <type_traits>

//namespace notf {

/** Custom smart pointer for sharing objects between Python and C++.
 * Interface is modeled after:
 *     https://github.com/wjakob/pybind11/blob/master/tests/object.h
 */
template <typename T> //, typename = std::enable_if_t<std::is_base_of<std::enable_shared_from_this<T>, T>::value>>
class python_ptr {
public:
    python_ptr() = default;
    ~python_ptr() = default;

    python_ptr(T* ptr)
        : m_strong()
        , m_weak()
    {
        try {
            m_strong = std::static_pointer_cast<T>(ptr->shared_from_this());
        }
        catch (const std::bad_weak_ptr&) {
            m_strong = std::make_shared<T>(); // TODO: this will contruct Foo at some point
        }
        m_weak = m_strong;
    }

    python_ptr(const std::shared_ptr<T>& ptr)
        : m_strong(ptr)
        , m_weak(m_strong)
    {
    }

    /** Copy constructor */
    python_ptr(const python_ptr& other)
        : m_strong(other.m_strong)
        , m_weak(other.m_weak)
    {
    }

    /** Move constructor */
    python_ptr(python_ptr&& other)
        : m_strong(other.m_strong)
        , m_weak(other.m_weak)
    {
        other.m_strong.reset();
        other.m_weak.reset();
    }

    /** Move another reference into the current one */
    python_ptr& operator=(python_ptr&& rhs)
    {
        if (*this == rhs) {
            return *this;
        }
        m_strong = rhs.m_strong;
        m_weak = rhs.m_weak;
        rhs.m_strong.reset();
        rhs.m_weak.reset();
        return *this;
    }

    /** Overwrite this reference with another reference */
    python_ptr& operator=(const python_ptr& rhs)
    {
        if (*this == rhs) {
            return *this;
        }
        m_strong = rhs.m_strong;
        m_weak = rhs.m_weak;
        return *this;
    }

    /** Overwrite this reference with a pointer to another object */
    python_ptr& operator=(const std::shared_ptr<T> ptr)
    {
        if (ptr == m_strong) {
            return *this;
        }
        m_strong = ptr;
        m_weak = ptr;
        return *this;
    }

    /** Compare this reference with another reference */
    bool operator==(const python_ptr& rhs) const { return m_weak.lock() == rhs.m_weak.lock(); }

    /** Compare this reference with another reference */
    bool operator!=(const python_ptr& rhs) const { return m_weak.lock() != rhs.m_weak.lock(); }

    /** Compare this reference with a pointer */
    bool operator==(const T* ptr) const { return get() == ptr; }

    /** Compare this reference with a pointer */
    bool operator!=(const T* ptr) const { return get() != ptr; }

    /** Access the object referenced by this reference */
    T* operator->() { return get(); }

    /** Access the object referenced by this reference */
    const T* operator->() const { return get(); }

    /** Return a C++ reference to the referenced object */
    T& operator*() { return *get(); }

    /** Return a const C++ reference to the referenced object */
    const T& operator*() const { return *get(); }

    /** Return a pointer to the referenced object */
    operator T*() { return get(); }

    /** Return a pointer to the referenced object */
    T* get()
    {
        if (auto locked = m_weak.lock()) {
            return locked.get();
        }
        assert(0);
        return nullptr;
    }

    /** Return a const pointer to the referenced object */
    const T* get() const
    {
        if (auto locked = m_weak.lock()) {
            return locked.get();
        }
        assert(0);
        return nullptr;
    }

    /** Turns this pointer from an owning to a weak pointer. */
    void decay()
    {
        assert(m_weak.lock() == m_strong);
        m_strong.release();
    }

public: // static fields
    typedef T element_type;

private: // fields
    /** Owning pointer to the referenced object, is valid until decay() is called. */
    std::shared_ptr<T> m_strong;

    /** Weak pointer to the referenced object, is always valid. */
    std::weak_ptr<T> m_weak;
};

//} // namespace notf
