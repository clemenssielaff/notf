#pragma once

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// utilities ======================================================================================================== //

/// No-op.
#define NOTF_NOOP ((void)0)

/// Interprets an expanded macro as a string.
#define NOTF_STR(x) #x

/// Takes two macros and concatenates them without whitespace in between.
#define NOTF_CONCAT(x, y) x##y

/// Expands a macro inside another macro.
/// Use for `NOTF_DEFER(NOTF_STR, expanded) and others
#define NOTF_DEFER(f, ...) f(__VA_ARGS__)

// object definition ================================================================================================ //

/// Convenience macro to disable the construction of automatic copy- and assign methods.
/// Implicitly disables move constructor/assignment methods as well, although you can define them yourself if you want.
#define NOTF_NO_COPY_OR_ASSIGN(Type) \
    Type(const Type&) = delete;      \
    void operator=(const Type&) = delete

/// Forbids the allocation on the heap of a given type.
#define NOTF_NO_HEAP_ALLOCATION(Type)      \
    void* operator new(size_t) = delete;   \
    void* operator new[](size_t) = delete; \
    void operator delete(void*) = delete;  \
    void operator delete[](void*) = delete

// forwards ========================================================================================================= //

/// Convenience macro to define shared pointer types for a given type.
#define NOTF_DEFINE_SHARED_POINTERS(Tag, Type)          \
    Tag Type;                                           \
    using Type##Ptr = std::shared_ptr<Type>;            \
    using Type##ConstPtr = std::shared_ptr<const Type>; \
    using Type##WeakPtr = std::weak_ptr<Type>;          \
    using Type##WeakConstPtr = std::weak_ptr<const Type>

/// Convenience macro to define unique pointer types for a given type.
#define NOTF_DEFINE_UNIQUE_POINTERS(Tag, Type) \
    Tag Type;                                  \
    using Type##Ptr = std::unique_ptr<Type>;   \
    using Type##ConstPtr = std::unique_ptr<const Type>

/// Convenience macro to define shared pointer types for a given templated type with one template argument.
#define NOTF_DEFINE_SHARED_POINTERS_TEMPLATE1(Tag, Type)   \
    template<class>                                        \
    Tag Type;                                              \
    template<class T>                                      \
    using Type##Ptr = std::shared_ptr<Type<T>>;            \
    template<class T>                                      \
    using Type##ConstPtr = std::shared_ptr<const Type<T>>; \
    template<class T>                                      \
    using Type##WeakPtr = std::weak_ptr<Type<T>>;          \
    template<class T>                                      \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T>>

/// Convenience macro to define unique pointer types for a given templated type with one template argument.
#define NOTF_DEFINE_UNIQUE_POINTERS_TEMPLATE1(Tag, Type) \
    template<class>                                      \
    Tag Type;                                            \
    template<class T>                                    \
    using Type##Ptr = std::unique_ptr<Type<T>>;          \
    template<class T>                                    \
    using Type##ConstPtr = std::unique_ptr<const Type<T>>

/// Convenience macro to define shared pointer types for a given templated type with two template arguments.
#define NOTF_DEFINE_SHARED_POINTERS_TEMPLATE2(Tag, Type)      \
    template<class, class>                                    \
    Tag Type;                                                 \
    template<class T, class U>                                \
    using Type##Ptr = std::shared_ptr<Type<T, U>>;            \
    template<class T, class U>                                \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U>>; \
    template<class T, class U>                                \
    using Type##WeakPtr = std::weak_ptr<Type<T, U>>;          \
    template<class T, class U>                                \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U>>

/// Convenience macro to define unique pointer types for a given templated type with two template arguments.
#define NOTF_DEFINE_UNIQUE_POINTERS_TEMPLATE2(Tag, Type) \
    template<class, class>                               \
    Tag Type;                                            \
    template<class T, class U>                           \
    using Type##Ptr = std::unique_ptr<Type<T, U>>;       \
    template<class T, class U>                           \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U>>

/// Convenience macro to define shared pointer types for a given templated type with three template arguments.
#define NOTF_DEFINE_SHARED_POINTERS_TEMPLATE3(Tag, Type)         \
    template<class, class, class>                                \
    Tag Type;                                                    \
    template<class T, class U, class V>                          \
    using Type##Ptr = std::shared_ptr<Type<T, U, V>>;            \
    template<class T, class U, class V>                          \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U, V>>; \
    template<class T, class U, class V>                          \
    using Type##WeakPtr = std::weak_ptr<Type<T, U, V>>;          \
    template<class T, class U, class V>                          \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U, V>>

/// Convenience macro to define unique pointer types for a given templated type with three template arguments.
#define NOTF_DEFINE_UNIQUE_POINTERS_TEMPLATE3(Tag, Type) \
    template<class, class, class>                        \
    Tag Type;                                            \
    template<class T, class U, class V>                  \
    using Type##Ptr = std::unique_ptr<Type<T, U, V>>;    \
    template<class T, class U, class V>                  \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U, V>>

/// Convenience macro to define shared pointer types for a given templated type with four template arguments.
#define NOTF_DEFINE_SHARED_POINTERS_TEMPLATE4(Tag, Type)            \
    template<class, class, class, class>                            \
    Tag Type;                                                       \
    template<class T, class U, class V, class W>                    \
    using Type##Ptr = std::shared_ptr<Type<T, U, V, W>>;            \
    template<class T, class U, class V, class W>                    \
    using Type##ConstPtr = std::shared_ptr<const Type<T, U, V, W>>; \
    template<class T, class U, class V, class W>                    \
    using Type##WeakPtr = std::weak_ptr<Type<T, U, V, W>>;          \
    template<class T, class U, class V, class W>                    \
    using Type##WeakConstPtr = std::weak_ptr<const Type<T, U, V, W>>

/// Convenience macro to define unique pointer types for a given templated type with four template arguments.
#define NOTF_DEFINE_UNIQUE_POINTERS_TEMPLATE4(Tag, Type) \
    template<class, class, class, class>                 \
    Tag Type;                                            \
    template<class T, class U, class V, class W>         \
    using Type##Ptr = std::unique_ptr<Type<T, U, V, W>>; \
    template<class T, class U, class V, class W>         \
    using Type##ConstPtr = std::unique_ptr<const Type<T, U, V, W>>

// compiler directives ============================================================================================== //

/// Tells the compiler that a given statement is likely to be evaluated to true.
#ifdef __GNUC__
#define NOTF_LIKELY(x) __builtin_expect(!!(x), 1)
#define NOTF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NOTF_LIKELY(x) (!!(x))
#define NOTF_UNLIKELY(x) (!!(x))
#endif

// raii ============================================================================================================= //

/// Uniquely named RAII guard object.
#define NOTF_GUARD(f) const auto NOTF_DEFER(NOTF_CONCAT, __notf__guard, __COUNTER__) = f

/// Define a guard object for a nested scope that is only aquired, if a given tests succeeds.
/// This macro simplifies double-checked locking, where we first test whether a condition is met before attempting to
/// lock a mutex. Note that this is only safe, if `x` is an atomic operation and `f` is used to initialize a mutex.
/// Example:
///     NOTF_GUARD_IF(m_atomic_bool, std::lock_guard(m_mutex)) {
///         ... // code here is only executed if m_atomic_bool is false and the mutex is locked
///     }
#define NOTF_GUARD_IF(x, f) \
    if (x)                  \
        if (NOTF_GUARD(f); x)

NOTF_CLOSE_NAMESPACE
