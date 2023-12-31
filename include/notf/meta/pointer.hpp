#pragma once

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/types.hpp"

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
static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// Template struct to test whether a given type is a unique pointer or not.
template<class T>
struct is_unique_ptr : std::false_type {};
template<class T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};
template<class T>
static constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// Template struct to test whether a given type is a valid pointer or not.
template<class T>
struct is_valid_ptr : std::false_type {};
template<class T>
struct is_valid_ptr<valid_ptr<T>> : std::true_type {};
template<class T>
static constexpr bool is_valid_ptr_v = is_valid_ptr<T>::value;

/// Checks if a pointer of type From can be cast to another of type To.
template<class From, class To, class = To>
struct is_static_castable : std::false_type {};
template<class From, class To>
struct is_static_castable<From, To, decltype(static_cast<To>(declval<From>()))> : std::true_type {};
template<class From, class To>
static constexpr bool is_static_castable_v = is_static_castable<From, To>::value;

// pointer support functions ======================================================================================== //

/// @{
/// Returns the raw pointer from any kind of other pointer without increasing the use_count of shared_ptrs.
template<class T>
constexpr std::enable_if_t<std::is_pointer_v<T>, T> raw_pointer(const T& ptr) noexcept {
    return ptr; // from raw
}
template<class T>
constexpr std::enable_if_t<is_shared_ptr_v<T>, typename T::element_type*> raw_pointer(const T& shared_ptr) noexcept {
    return shared_ptr.get(); // from shared_ptr<T>
}
template<class T>
constexpr std::enable_if_t<is_unique_ptr_v<T>, typename T::element_type*> raw_pointer(const T& unique_ptr) noexcept {
    return unique_ptr.get(); // from unique_ptr<T>
}
template<class T>
constexpr auto raw_pointer(const T& valid_ptr) noexcept
    // not `enable_if_t` ... MSVC seems to require the manual `typename` here
    -> decltype(typename std::enable_if<is_valid_ptr_v<T>>::type(), raw_pointer(valid_ptr.get())) {
    return raw_pointer(valid_ptr.get()); // from valid_ptr<T>
}
template<class T>
constexpr std::enable_if_t<std::is_null_pointer_v<T>, std::nullptr_t> raw_pointer(T) noexcept {
    return nullptr; // from nullptr
}
/// @}

/// Checks if a given type contains a raw pointer that can be extracted with `raw_pointer`.
template<class T, class = void>
struct has_raw_pointer : std::false_type {};
template<class T>
struct has_raw_pointer<T, decltype(raw_pointer(declval<T>()))> : std::true_type {};

/// Cast a pointer of type From to a pointer of Type To and raises an assert if a dynamic_cast fails.
/// If a static_cast is possible, it will be performed and always succeed.
template<class From, class To>
constexpr std::enable_if_t<std::conjunction_v<std::is_pointer<From>, std::is_pointer<To>>, To>
assert_cast(From castee) noexcept(is_static_castable_v<From, To>) {
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

// weak pointer functions =========================================================================================== //

/// Compares two weak_ptrs without locking them.
/// You can have the first argument provide the common type or explicitly define one,
/// for example if both a and b are (potenially different) subclasses of T.
/// @param a    First weak_ptr.
/// @param b    Second weak_ptr.
template<class T>
bool is_weak_ptr_equal(const std::weak_ptr<T>& a, const identity_t<std::weak_ptr<T>>& b) noexcept {
    return !a.owner_before(b) && !b.owner_before(a);
}

/// Checks if a weak_ptr is empty.
/// @param ptr  Weak pointer to test
/// @returns    True iff the weak pointer is empty (not initialized).
template<class T>
bool is_weak_ptr_empty(const std::weak_ptr<T>& ptr) noexcept {
    return is_weak_ptr_equal(ptr, std::weak_ptr<T>{});
}

/// Extracts the raw pointer from a std::weak_ptr.
/// This is certainly not portable and safe... but it works for tested versions for libc++ and libstdc++ and msvc++.
/// This can be removed as soon as `owner_hash` becomes part of the c++ standard
/// (http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-active.html#1406)
template<class T>
const T* raw_from_weak_ptr(const std::weak_ptr<T>& weak_ptr) noexcept {
    struct ConceptualWeakPtr {
        const T* element;
        const void* control_block;
    };
    return std::launder(reinterpret_cast<const ConceptualWeakPtr*>(&weak_ptr))->element;
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

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of the value pointed to by this value_ptr.
    using element_type = std::remove_pointer_t<T>;

    /// Error thrown by valid_ptr, when you try to dereference a nullptr.
    NOTF_EXCEPTION_TYPE(NotValidError);

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor
    /// @param ptr  Pointer to wrap.
    /// @throws NotValidError   If you try to stantiate with a nullptr.
    template<class From, typename = std::enable_if_t<std::conjunction_v<        //
                             std::is_convertible<From, T>,                      // ptr must be convertible to T,
                             std::negation<is_valid_ptr<std::decay_t<From>>>>>> // but not a valid_ptr itself
    constexpr valid_ptr(From&& ptr) : m_ptr(std::forward<From>(ptr)) {
        if (NOTF_UNLIKELY(m_ptr == nullptr)) { NOTF_THROW(NotValidError, "Failed to dereference an empty pointer"); }
    }

    /// Safe copy constructor
    /// @param  Other valid_ptr containing a convertible type.
    template<class From, typename = std::enable_if_t<std::is_convertible_v<From, T>>>
    constexpr valid_ptr(const valid_ptr<From>& ptr) : m_ptr(ptr.m_ptr) {}

    // default construction for other valid_ptr<T>
    valid_ptr(const valid_ptr& other) = default;
    valid_ptr(valid_ptr&& other) noexcept = default;
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

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The wrapped pointer.
    T m_ptr;
};

// pointer comparisons ============================================================================================== //

/// Functor to use when comparing pointers of different types (raw, valid_ptr, shared_ptr, unique_ptr).
/// From: https://stackoverflow.com/a/18940595
struct pointer_equal {
    using is_transparent = std::true_type;

    /// Comparison operation.
    template<class T, class U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
        return raw_pointer(lhs) == raw_pointer(rhs);
    }
};
struct pointer_less_than {
    using is_transparent = std::true_type;

    /// Comparison operation.
    template<class T, class U>
    constexpr bool operator()(const T& lhs, const U& rhs) const noexcept {
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
constexpr bool operator==(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) == raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator!=(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) != raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator<(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) < raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator<=(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) <= raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator>(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) > raw_pointer(std::forward<R>(rhs));
}
template<class L, class R, typename = detail::is_pointer_comparable<L, R>>
constexpr bool operator>=(L&& lhs, R&& rhs) noexcept {
    return raw_pointer(std::forward<L>(lhs)) >= raw_pointer(std::forward<R>(rhs));
}

NOTF_CLOSE_NAMESPACE
