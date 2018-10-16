#pragma once

#include "./assert.hpp"
#include "./exception.hpp"
#include "./hash.hpp"

NOTF_OPEN_NAMESPACE

// forwards
template<class T>
class valid_ptr;

// pointer trait support ============================================================================================ //

/// Template struct to test whether a given type is a shared pointer or not.
template<class T>
struct is_shared_ptr : std::false_type {};
template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template<class T>
static constexpr const bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// Template struct to test whether a given type is a unique pointer or not.
template<class T>
struct is_unique_ptr : std::false_type {};
template<class T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};
template<class T>
static constexpr const bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// Template struct to test whether a given type is a valid pointer or not.
template<class T>
struct is_valid_ptr : std::false_type {};
template<class T>
struct is_valid_ptr<valid_ptr<T>> : std::true_type {};
template<class T>
static constexpr const bool is_valid_ptr_v = is_valid_ptr<T>::value;

/// Checks if a pointer of type From can be cast to another of type To.
template<class From, class To, class = To>
struct is_static_castable : std::false_type {};
template<class From, class To>
struct is_static_castable<From, To, decltype(static_cast<To>(std::declval<From>()))> : std::true_type {};
template<class From, class To>
static constexpr const bool is_static_castable_v = is_static_castable<From, To>::value;

// pointer support functions ======================================================================================== //

/// @{
/// Returns the raw pointer from any kind of other pointer without increasing the use_count of shared_ptrs.
template<class T>
constexpr std::enable_if_t<std::is_pointer_v<T>, T> raw_pointer(const T& ptr) noexcept
{
    return ptr; // from raw
}
template<class T>
constexpr std::enable_if_t<is_shared_ptr_v<T>, typename T::element_type*> raw_pointer(const T& shared_ptr) noexcept
{
    return shared_ptr.get(); // from shared_ptr<T>
}
template<class T>
constexpr std::enable_if_t<is_unique_ptr_v<T>, typename T::element_type*> raw_pointer(const T& unique_ptr) noexcept
{
    return unique_ptr.get(); // from unique_ptr<T>
}
template<class T>
constexpr std::enable_if_t<is_valid_ptr_v<T>, typename T::element_type*> raw_pointer(const T& valid_ptr) noexcept
{
    return raw_pointer(valid_ptr.get()); // from valid_ptr<T>
}
template<class T>
constexpr std::enable_if_t<std::is_null_pointer_v<T>, std::nullptr_t> raw_pointer(T) noexcept
{
    return nullptr; // from nullptr
}
/// @}

/// Checks if a given type contains a raw pointer that can be extracted with `raw_pointer`.
template<class T, typename = decltype(raw_pointer(std::declval<T>()))>
struct has_raw_pointer : std::true_type {};

/// Cast a pointer of type From to a pointer of Type To and raises an assert if a dynamic_cast fails.
/// If a static_cast is possible, it will be performed and always succeed.
template<class From, class To>
constexpr std::enable_if_t<std::conjunction_v<std::is_pointer<From>, std::is_pointer<To>>, To>
assert_cast(From castee) noexcept(is_static_castable_v<From, To>)
{
    // static cast
    if constexpr (is_static_castable_v<From, To>) {
        return static_cast<To>(castee);
    }

    // dynamic cast
    else {
#ifdef NOTF_DEBUG
        auto result = dynamic_cast<To>(raw_pointer(castee));
        NOTF_ASSERT(result);
        return result;
#else
        return static_cast<To>(castee);
#endif
    }
}

/// Specialized Hash for pointers.
/// Uses `hash_mix` to improve pointer entropy.
template<typename T>
struct pointer_hash {
    size_t operator()(const T& ptr) const noexcept { return hash_mix(to_number(raw_pointer(ptr))); }
};

// weak pointer functions =========================================================================================== //

/// Compares two weak_ptrs without locking them.
/// You can have the first argument provide the common type or explicitly define one,
/// for example if both a and b are (potenially different) subclasses of T.
/// @param a    First weak_ptr.
/// @param b    Second weak_ptr.
template<class T>
bool weak_ptr_equal(const std::weak_ptr<T>& a, const identity_t<std::weak_ptr<T>>& b)
{
    return !a.owner_before(b) && !b.owner_before(a);
}

/// Checks if a weak_ptr is empty.
/// @param ptr  Weak pointer to test
/// @returns    True iff the weak pointer is empty (not initialized).
template<class T>
bool weak_ptr_empty(const std::weak_ptr<T>& ptr)
{
    return weak_ptr_equal(ptr, std::weak_ptr<T>{});
}

// safe(r) pointer types ============================================================================================ //

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
/// See GSL for license.
template<class T>
class valid_ptr {

    static_assert(std::is_assignable_v<T&, std::nullptr_t>, "T cannot become a null pointer.");

    template<class>
    friend class valid_ptr; // valid_ptrs have access to each other's `m_ptr` field.

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Type of the value pointed to by this value_ptr.
    using element_type = std::remove_pointer_t<T>;

    /// Error thrown by risky_ptr, when you try to dereference a nullptr.
    NOTF_EXCEPTION_TYPE(invalid_pointer_error);

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor
    /// @param ptr  Pointer to wrap.
    /// @throws invalid_pointer_error   If you try to instantiate with a nullptr.
    template<class From, typename = std::enable_if_t<std::conjunction_v<        //
                             std::is_convertible<From, T>,                      // ptr must be convertible to T,
                             std::negation<is_valid_ptr<std::decay_t<From>>>>>> // but not a valid_ptr itself
    constexpr valid_ptr(From&& ptr) : m_ptr(std::forward<From>(ptr))
    {
        if (NOTF_UNLIKELY(m_ptr == nullptr)) {
            NOTF_THROW(invalid_pointer_error, "Failed to dereference an empty pointer");
        }
    }

    /// Safe copy constructor
    /// @param  Other valid_ptr containing a convertible type.
    template<class From, typename = std::enable_if_t<std::is_convertible_v<From, T>>>
    constexpr valid_ptr(const valid_ptr<From>& ptr) : m_ptr(ptr.m_ptr)
    {}

    // default construction for other valid_ptr<T>
    valid_ptr(const valid_ptr& other) = default;
    valid_ptr(valid_ptr&& other) = default;
    valid_ptr& operator=(const valid_ptr& other) = default;

    // forbid empty or construction with nullptr
    valid_ptr() = delete;
    valid_ptr(std::nullptr_t) = delete;
    valid_ptr& operator=(std::nullptr_t) = delete;

    /// Pointer access.
    /// @{
    constexpr T get() noexcept { return m_ptr; }
    constexpr const T get() const noexcept { return m_ptr; }
    constexpr operator T() noexcept { return m_ptr; }
    constexpr T& operator->() noexcept { return m_ptr; }
    constexpr const T& operator->() const noexcept { return m_ptr; }
    constexpr decltype(auto) operator*() noexcept { return *m_ptr; }
    constexpr decltype(auto) operator*() const noexcept { return *m_ptr; }
    /// @}

    // remove unwanted pointer arithmetic
    valid_ptr& operator++() = delete;
    valid_ptr& operator--() = delete;
    template<class Other>
    valid_ptr operator+(Other) = delete;
    template<class Other>
    valid_ptr operator-(Other) = delete;
    template<class Other>
    valid_ptr operator++(Other) = delete;
    template<class Other>
    valid_ptr operator--(Other) = delete;
    template<class Other>
    valid_ptr& operator+=(Other) = delete;
    template<class Other>
    valid_ptr& operator-=(Other) = delete;
    template<class Other>
    void operator[](Other) const = delete;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The wrapped pointer.
    T m_ptr;
};

/// Denote a raw pointer as "risky".
/// Doesn't add any functionality, only signifies to the reader that a function may return nullptr.
template<class T>
using risky_ptr = std::decay_t<T>*;

// pointer comparisons ============================================================================================== //

/// Functor to use when comparing pointers of different types (raw, valid_ptr, risky_ptr, shared_ptr, unique_ptr).
/// From: https://stackoverflow.com/a/18940595
struct pointer_equal {
    using is_transparent = std::true_type;

    /// Comparison operation.
    template<class T, class U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept
    {
        return raw_pointer(lhs) == raw_pointer(rhs);
    }
};
struct pointer_less_than {
    using is_transparent = std::true_type;

    /// Comparison operation.
    template<class T, class U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept
    {
        return raw_pointer(lhs) < raw_pointer(rhs);
    }
};

namespace detail {
template<class L, class R>
using is_pointer_comparable = std::enable_if_t<std::conjunction_v<has_raw_pointer<std::decay_t<L>>, //
                                                                  has_raw_pointer<std::decay_t<R>>>>;
} // namespace detail

/// Free comparison functions
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator==(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) == raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator!=(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) != raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator<(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) < raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator<=(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) <= raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator>(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) > raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator>=(L&& lhs, R&& rhs) noexcept
{
    return raw_pointer(std::forward<L>(lhs)) >= raw_pointer(std::forward<R>(rhs));
}

NOTF_CLOSE_NAMESPACE
