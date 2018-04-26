#pragma once

#include "common/exception.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Error thrown by risky_ptr, when you try to dereference a nullptr.
NOTF_EXCEPTION_TYPE(bad_pointer_error);

//====================================================================================================================//

/// Helper struct to use when comparing pointers of different types (raw, valid_ptr, risky_ptr, shared_ptr, unique_ptr).
/// From:
///     https://stackoverflow.com/a/18940595
template<class T>
struct pointer_equal {
    typedef std::true_type is_transparent;

    /// Helps reducing the number of comparison pairs.
    struct Helper {

        /// Constructor overload for raw pointers.
        /// @param ptr  Raw pointer.
        Helper(T* ptr) : m_ptr(ptr) {}

        /// Constructor overload for all types that make a pointer available using `get()`.
        /// @param ptr  Some other pointer type.
        template<typename U>
        Helper(const U& ptr) : m_ptr(ptr.get())
        {}

        /// Comparison operator.
        /// @param other    Other pointer to compare against.
        bool operator==(const Helper& other) const { return m_ptr == other.m_ptr; }

    private:
        /// Pointer.
        T* m_ptr = nullptr;
    };

    /// Comparison operation.
    /// Transforms both sides into the internal `Helper` type to avoid manual overloads for all kinds of pointers.
    bool operator()(Helper&& lhs, Helper&& rhs) const { return lhs == rhs; }
};

//====================================================================================================================//

/// Restricts a pointer or smart pointer to only hold non-null values.
/// Has zero size overhead over T.
///
/// If T is a pointer (i.e. T == U*) then
/// - allow construction from U*
/// - disallow construction from nullptr_t
/// - disallow default construction
/// - ensure construction from null U* fails
/// - allow implicit conversion to U*
///
/// Adapted from:
///     https://github.com/Microsoft/GSL/blob/master/include/gsl/pointers
/// (See copyright at the end of the class).
template<typename T>
struct valid_ptr {

    static_assert(std::is_assignable<T&, std::nullptr_t>::value, "T cannot be assigned nullptr.");

    /// No default constructor.
    valid_ptr() = delete;

    /// Value Constructor
    /// @param ptr  Pointer to wrap.
    /// @throws bad_pointer_error   If you try to instantiate with a nullptr.
    template<typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr valid_ptr(U&& u) : m_ptr(std::forward<U>(u))
    {
        if (NOTF_UNLIKELY(m_ptr == nullptr)) {
            notf_throw(bad_pointer_error, "Failed to dereference an empty pointer");
        }
    }

    /// Copy Constructor.
    /// @param other    Other pointer to copy.
    template<typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr valid_ptr(const valid_ptr<U>& other) : valid_ptr(other.get())
    {}

    valid_ptr(valid_ptr&& other) = default;
    valid_ptr(const valid_ptr& other) = default;
    valid_ptr& operator=(const valid_ptr& other) = default;

    /// @{
    /// The wrapped pointer.
    /// @throws bad_pointer_error   If the wrapped pointer is empty.
    constexpr T get() const
    {
        if (NOTF_UNLIKELY(m_ptr == nullptr)) {
            notf_throw(bad_pointer_error, "Failed to dereference an empty pointer");
        }
        return m_ptr;
    }
    constexpr operator T() const { return get(); }
    constexpr T operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }
    /// @}

    // prevents compilation when someone attempts to assign a null pointer constant
    valid_ptr(std::nullptr_t) = delete;
    valid_ptr& operator=(std::nullptr_t) = delete;

    // remove unwanted pointer arithmetic
    valid_ptr& operator++() = delete;
    valid_ptr& operator--() = delete;
    valid_ptr operator++(int) = delete;
    valid_ptr operator--(int) = delete;
    valid_ptr& operator+=(std::ptrdiff_t) = delete;
    valid_ptr& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    T m_ptr;
};
///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

template<class T>
std::ostream& operator<<(std::ostream& os, const valid_ptr<T>& val)
{
    os << val.get();
    return os;
}

template<class T, class U>
auto operator==(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template<class T, class U>
auto operator!=(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template<class T, class U>
auto operator<(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template<class T, class U>
auto operator<=(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template<class T, class U>
auto operator>(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template<class T, class U>
auto operator>=(const valid_ptr<T>& lhs, const valid_ptr<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators for `valid_ptr`
template<class T, class U>
std::ptrdiff_t operator-(const valid_ptr<T>&, const valid_ptr<U>&) = delete;
template<class T>
valid_ptr<T> operator-(const valid_ptr<T>&, std::ptrdiff_t) = delete;
template<class T>
valid_ptr<T> operator+(const valid_ptr<T>&, std::ptrdiff_t) = delete;
template<class T>
valid_ptr<T> operator+(std::ptrdiff_t, const valid_ptr<T>&) = delete;

//====================================================================================================================//

/// Pointer wrapper to make sure that if a function can return a nullptr, the user either checks it before dereferencing
/// or doesn't use it.
/// Similar to `valid_ptr`, but only checks if it is valid, when you dereference it.
template<typename T>
struct risky_ptr {

    static_assert(std::is_assignable<T&, std::nullptr_t>::value, "T cannot be assigned nullptr.");

    /// Default constructor.
    risky_ptr() = default;

    /// Value Constructor
    /// @param ptr  Pointer to wrap.
    template<typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr risky_ptr(U&& u) : m_ptr(std::forward<U>(u))
    {}

    /// Copy Constructor.
    /// @param other    Other pointer to copy.
    template<typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr risky_ptr(const risky_ptr<U>& other) : risky_ptr(other.get())
    {}

    risky_ptr(risky_ptr&& other) = default;
    risky_ptr(const risky_ptr& other) = default;
    risky_ptr& operator=(const risky_ptr& other) = default;

    /// @{
    /// Access to the wrapped pointer.
    /// @throws bad_pointer_error   If the wrapped pointer is empty.
    constexpr T get() const
    {
        if (NOTF_UNLIKELY(m_ptr == nullptr)) {
            notf_throw(bad_pointer_error, "Failed to dereference an empty pointer");
        }
        return m_ptr;
    }
    constexpr operator T() const { return get(); }
    constexpr T operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }
    /// @}

    /// Tests if the contained pointer is save.
    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    /// Tests if the contained pointer is empty.
    bool operator!() const noexcept { return m_ptr == nullptr; }

    // remove unwanted pointer arithmetic
    risky_ptr& operator++() = delete;
    risky_ptr& operator--() = delete;
    risky_ptr operator++(int) = delete;
    risky_ptr operator--(int) = delete;
    risky_ptr& operator+=(std::ptrdiff_t) = delete;
    risky_ptr& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Wrapped pointer.
    T m_ptr = nullptr;
};

template<class T>
std::ostream& operator<<(std::ostream& os, const risky_ptr<T>& val)
{
    os << val.get();
    return os;
}

template<class T, class U>
auto operator==(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template<class T, class U>
auto operator!=(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template<class T, class U>
auto operator<(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template<class T, class U>
auto operator<=(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template<class T, class U>
auto operator>(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template<class T, class U>
auto operator>=(const risky_ptr<T>& lhs, const risky_ptr<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators for `risky_ptr`
template<class T, class U>
std::ptrdiff_t operator-(const risky_ptr<T>&, const risky_ptr<U>&) = delete;
template<class T>
risky_ptr<T> operator-(const risky_ptr<T>&, std::ptrdiff_t) = delete;
template<class T>
risky_ptr<T> operator+(const risky_ptr<T>&, std::ptrdiff_t) = delete;
template<class T>
risky_ptr<T> operator+(std::ptrdiff_t, const risky_ptr<T>&) = delete;

NOTF_CLOSE_NAMESPACE

//====================================================================================================================//

namespace std {

template<class T>
struct hash<notf::valid_ptr<T>> {
    std::size_t operator()(const notf::valid_ptr<T>& value) const { return hash<T>{}(value); }
};

template<class T>
struct hash<notf::risky_ptr<T>> {
    std::size_t operator()(const notf::risky_ptr<T>& value) const { return hash<T>{}(value); }
};

} // namespace std
