#pragma once

#include <limits.h>
#include <memory>

// ================================================================================================================== //

/// Compiler detection.
#ifdef __clang__
#define NOTF_CLANG
#else
#ifdef _MSC_VER
#define NOTF_MSVC
#else
#ifdef __GNUC__
#define NOTF_GCC
#else
#error Unknown compiler detected (searching for __clang__, _MSC_VER and __GNUC__)
#endif
#endif
#endif

// ================================================================================================================== //

/// OS detection
#ifdef __linux__
#define NOTF_LINUX
#else
#ifdef _WIN32
#define NOTF_WINDOWS
#else
#error Unknown operating system detected (searching for __linux__ and _WIN32)
#endif
#endif

// ================================================================================================================== //

/// Function name macro to use for logging and exceptions.
#ifdef NOTF_LOG_PRETTY_FUNCTION
#ifdef NOTF_CLANG
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#ifdef NOTF_MSVC
#define NOTF_CURRENT_FUNCTION __FUNCTION__
#else
#ifdef NOTF_GCC
#define NOTF_CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif
#endif
#endif
#else
#define NOTF_CURRENT_FUNCTION __func__
#endif

// ================================================================================================================== //

/// Pragma support for macros.
#ifdef NOTF_MSVC
#define NOTF_PRAGMA(x) __pragma(x)
#else
#define NOTF_PRAGMA(x) _Pragma(x)
#endif

/// Nothing happening here.
#define NOTF_NOOP ((void)0)

/// Compiler attribute detection, as described in:
///     https://clang.llvm.org/docs/LanguageExtensions.html#has-cpp-attribute
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) (0)
#endif

/// NOTF_NODISCARD attribute to make sure that the return value of a function is not immediately discarded.
/// Example:
///     NOTF_NODISCARD int guard() { ...
///     class NOTF_NODISCARD Guard { ...
#if !defined NOTF_NODISCARD
#if __has_cpp_attribute(nodiscard)
#define NOTF_NODISCARD [[nodiscard]]
#elif __has_cpp_attribute(gnu::warn_unused_result)
#define NOTF_NODISCARD [[gnu::warn_unused_result]]
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define NOTF_NODISCARD _Check_return_
#elif defined(__clang__) || defined(__GNUC__)
#define NOTF_NODISCARD __attribute__((__warn_unused_result__))
#else
#define NOTF_NODISCARD
#endif
#endif

/// Signifies that a value is (probably) unused and you don't want warnings about it.
/// Example:
///     NOTF_UNUSED int answer = 42;
#if __has_cpp_attribute(maybe_unused)
#define NOTF_UNUSED [[maybe_unused]]
#elif __has_cpp_attribute(gnu::unused)
#define NOTF_UNUSED [[gnu::unused]]
#else
#define NOTF_UNUSED
#endif

/// Signifies that the function will not return control flow back to the caller.
#if __has_cpp_attribute(noreturn)
#define NOTF_NORETURN [[noreturn]]
#else
#define NOTF_NORETURN
#endif

/// Tells the compiler that a given statement is likely to be evaluated to true.
#ifdef __GNUC__
#define NOTF_LIKELY(x) __builtin_expect(!!(x), 1)
#define NOTF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NOTF_LIKELY(x) (!!(x))
#define NOTF_UNLIKELY(x) (!!(x))
#endif

// ================================================================================================================== //

/// Interprets an expanded macro as a string.
#define NOTF_STR(x) #x

/// Takes two macros and concatenates them without whitespace in between.
#define NOTF_CONCAT(x, y) x##y

/// Expands a macro inside another macro.
/// Use for `NOTF_DEFER(NOTF_STR, expanded) and others
#define NOTF_DEFER(f, ...) f(__VA_ARGS__)

/// Helper to add a comma before __VA_ARGS__ but only if the list is not empty
#define NOTF_VA_ARGS(...) , ##__VA_ARGS__

// ================================================================================================================== //

/// Convienience macros to temporarely disable a single warning.
/// NOTF_DISABLE_WARNING("switch-enum")
///     ...
/// NOTF_ENABLE_WARNINGS
#define NOTF_DISABLE_WARNING_STR_(x) GCC diagnostic ignored "-W" NOTF_STR(x)
#define NOTF_SAVE_DIAGNOSTICS NOTF_PRAGMA("clang diagnostic push")
#define NOTF_DISABLE_WARNING(x) NOTF_PRAGMA(NOTF_DEFER(NOTF_STR, NOTF_DISABLE_WARNING_STR_(x)))
#define NOTF_RESTORE_DIAGNOSTICS NOTF_PRAGMA("clang diagnostic pop")

/// Print a compiler warning with some additional information.
#define NOTF_COMPILER_WARNING(x) \
    NOTF_PRAGMA(NOTF_DEFER(NOTF_STR, warning "(warning) on line " NOTF_DEFER(NOTF_STR, __LINE__) ": " x))

// ================================================================================================================== //

/// This macro can later be used to implement namespace versioning.
#define NOTF_NAMESPACE_NAME notf

/// Opens the NoTF namespace.
#define NOTF_OPEN_NAMESPACE namespace NOTF_NAMESPACE_NAME {

/// For visual balance with NOTF_OPEN_NAMESPACE.
#define NOTF_CLOSE_NAMESPACE }

/// Use the versioned namespace.
#define NOTF_USING_NAMESPACE using namespace NOTF_NAMESPACE_NAME

// ================================================================================================================== //

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

// ================================================================================================================== //

NOTF_OPEN_NAMESPACE

/// Helper to reduce cv-qualified pointers/references to their base type.
template<class T, class U = std::remove_reference_t<std::remove_pointer_t<std::remove_cv_t<T>>>>
struct strip_type {
    using type = U;
};
template<class T>
using strip_type_t = typename strip_type<T>::type;

/// Type template to ensure that a template argument does not participate in type deduction.
/// Compare:
///
///     template <class T>
///     void multiply_each(std::vector<T>& vec, T factor) {
///         for (auto& item : vec){
///             item *= factor;
///         }
///     }
///     std::vector<long> vec{1, 2, 3};
///     multiply_each(vec, 5);  // error: no matching function call to
///                             // multipy_each(std::vector<long int>&, int)
///                             // note: template argument deduction/substitution failed:
///                             // note: deduced conflicting types for parameter `T` (`long int` and `int`)
///
/// with:
///
///     template <class T>
///     void multiply_each(std::vector<T>& vec, identity_t<T> factor)
///     {
///         for (auto& item : vec){
///             item *= factor;
///         }
///     }
///     std::vector<long> vec{1, 2, 3};
///     multiply_each(vec, 5);  // works
///
template<class T>
struct identity {
    using type = T;
};
template<class T>
using identity_t = typename identity<T>::type;

// ================================================================================================================== //

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

// ================================================================================================================== //

// use cool names, just in case your compiler isn't cool enough
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ulonglong = unsigned long long;

// ================================================================================================================== //

namespace test {
/// Special type used by tests to access Type::Access<test::Harness> instances defined by NOTF_ACESS_TYPES(...).
/// Is never actually defined.
struct Harness;
} // namespace test

// ================================================================================================================== //

/// Checks if T is any of the variadic types.
template<class T, class... Ts>
using is_one_of = std::disjunction<std::is_same<T, Ts>...>;

/// The `always_false` struct can be used to create a templated struct that will always evaluate to `false` when
/// used in a static_assert.
///
/// Imagine the following scenario: You have a scene graph and a fairly generic operation `calc` on some object class,
/// that you want to be performed in various spaces:
///
///     enum class Space {
///        WORLD,
///        LOCAL,
///        SCREEN,
///     };
///
/// Instead of passing a `Space` parameter at runtime, or cluttering the interface with `calc_in_world` and
/// `calc_in_local` etc. -operations, you want a template method: `calc<Space>()`. Then you can specialize the template
/// method for various spaces. If a space is *not* supported, you can just not specialize the method for it. The
/// original template method would look this this:
///
///     template<Space space>
///     float calc() {
///         static_assert(always_false<Space, space>{}, "Unsupported Space");
///     }
///
/// And specializations like this:
///
///     template<>
///     float calc<Space:WORLD>() { return 0; }
///
/// You can now call supported specializations, but unsupported ones will fail at runtime:
///
///     calc<Space::WORLD>();  // produces 0
///     calc<Space::SCREEN>(); // error! static assert: "Unsupported Space"
///
template<class T, T val>
struct always_false : std::false_type {};

/// Like always_false, but taking a single type only.
/// This way you can define error overloads that differ by type only:
///
///     static void convert(const Vector4d& in, std::array<float, 4>& out)
///     {
///         ...
///     }
///
///     template <class UNSUPPORTED_TYPE>
///     static void convert(const Vector4d& in, UNSUPPORTED_TYPE& out)
///     {
///         static_assert(always_false_t<T>{}, "Cannot convert a Vector4d into template type T");
///     }
///
template<class T>
struct always_false_t : std::false_type {};

/// Compile-time check whether two types are both signed / both unsigned.
template<class T, class U>
struct is_same_signedness : public std::integral_constant<bool, std::is_signed<T>::value == std::is_signed<U>::value> {
};

// ================================================================================================================== //

/// Member detector idiom as described on:
///     https://en.wikibooks.org/wiki/More_C++_Idioms/Member_Detector
///
/// Usage:
///     NOTF_HAS_MEMBER_DETECTOR(foo);  // generates a detector type `has_member_foo`
///     has_member_foo<Bar>::value;     // is either true or false
///
/// Note that the class is NOT PUT INTO A NAMESPACE, so you can use it inside a class body as well.
/// In the wild, if you don't put it into a class body, consider putting the generated class into the `detail` (or
/// whatever) namespace.
#define NOTF_HAS_MEMBER_DETECTOR(Member)                                              \
    template<class T>                                                                 \
    class _HasMember_##Member {                                                       \
    private:                                                                          \
        using No = char[1];                                                           \
        using Yes = char[2];                                                          \
        struct Fallback {                                                             \
            int member;                                                               \
        };                                                                            \
        struct Derived : T, Fallback {};                                              \
        template<class U>                                                             \
        static No& test(decltype(U::member)*);                                        \
        template<class U>                                                             \
        static Yes& test(U*);                                                         \
                                                                                      \
    public:                                                                           \
        static constexpr bool RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes); \
    };                                                                                \
    template<class T>                                                                 \
    struct has_member_##Member : public std::integral_constant<bool, _HasMember_##Member<T>::RESULT> {}

/// Like NOTF_HAS_MEMBER_DETECTOR but for detecting the presence of types.
#define NOTF_HAS_MEMBER_TYPE_DETECTOR(Type)                                           \
    template<class T>                                                                 \
    class _HasMemberType_##Type {                                                     \
    private:                                                                          \
        using No = char[1];                                                           \
        using Yes = char[2];                                                          \
        struct Fallback {                                                             \
            struct Type {};                                                           \
        };                                                                            \
        struct Derived : T, Fallback {};                                              \
        template<class U>                                                             \
        static No& test(typename U::Type*);                                           \
        template<class U>                                                             \
        static Yes& test(U*);                                                         \
                                                                                      \
    public:                                                                           \
        static constexpr bool RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes); \
    };                                                                                \
                                                                                      \
    template<class T>                                                                 \
    struct has_member_type_##Type : public std::integral_constant<bool, _HasMemberType_##Type<T>::RESULT> {}

/// Is only a valid expression if the given type exists.
/// Can be used to check whether a template type has a certain nested type:
///
///     template <typename T, typename = void>
///     constexpr bool has_my_type = false;
///     template <typename T>
///     constexpr bool has_my_type<T, decltype(check_type<typename T::has_my_type>(), void())> = true;
///
template<class T>
constexpr void check_type()
{
    static_cast<void>(typeid(T));
}

// ================================================================================================================== //

namespace detail {
template<class T>
constexpr void _notf_is_constexpr_helper(T&&)
{}
} // namespace detail

/// Macro to test whether a function call is available at compile time or not.
/// From: https://stackoverflow.com/a/46920091
#define NOTF_IS_CONSTEXPR(...) noexcept(detail::_notf_is_constexpr_helper(__VA_ARGS__))

// ================================================================================================================== //

/// Constexpr to use an enum class value as a numeric value.
/// From "Effective Modern C++ by Scott Mayers': Item #10.
template<class Enum, class = std::enable_if_t<std::is_enum<Enum>::value>>
constexpr auto to_number(Enum enumerator) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(enumerator);
}

/// Converts any pointer to the equivalent integer representation.
template<class T, class = std::enable_if_t<std::is_pointer<T>::value>>
constexpr std::uintptr_t to_number(const T ptr) noexcept
{
    return reinterpret_cast<std::uintptr_t>(ptr);
}

// ================================================================================================================== //

/// Convenience method to use as
///     if(all(a, !b, c < d)){ // is equal to ```if(a && !b && c < d){```
template<class... Ts>
constexpr bool all(Ts... expressions)
{
    return (... && expressions);
}

// ================================================================================================================== //

/// Defined not because boolean reasoning is not hard.
#define NOTF_THROWS_IF(x) noexcept(!(x))
#define NOTF_THROWS noexcept(false)

/// Useful for cases like `NOTF_THROWS_IF(is_debug_build())`.
#ifdef NOTF_DEBUG
constexpr bool is_debug_build() noexcept { return true; }
#else
constexpr bool is_debug_build() noexcept { return false; }
#endif

// ================================================================================================================== //

namespace detail {

/// Simple `new` forward to allow the creation of an instance from a protected or private constructor.
template<class T, class... Args>
inline T* make_new_enabler(Args&&... args)
{
    return new T(std::forward<Args>(args)...);
}

/// Helper struct to allow `std::make_shared` to work with protected or private constructors.
/// from:
///     https://stackoverflow.com/a/8147213 and https://stackoverflow.com/a/25069711
template<class T>
struct make_shared_enabler : public T {
    template<class... Args>
    make_shared_enabler(Args&&... args) : T(std::forward<Args>(args)...)
    {}
};

} // namespace detail

/// Put into the class body to allow the use of NOTF_MAKE_SHARED_FROM_PRIVATE
#define NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE           \
    template<class _T, class... _Args>               \
    friend _T* detail::make_new_enabler(_Args&&...); \
    template<class>                                  \
    friend struct detail::make_shared_enabler

/// I found that debugging a `detail::make_shared_enable<T>` instance is not as reliable (members seem to be missing?)
/// as when you create a shared pointer from a raw pointer. Therefore, NOTF_MAKE_SHARED_FROM_PRIVATE works differently
/// in debug and release mode.
#ifdef NOTF_DEBUG
#define NOTF_MAKE_SHARED_FROM_PRIVATE(T, ...) std::shared_ptr<T>(detail::make_new_enabler<T>(__VA_ARGS__))
#define NOTF_MAKE_UNIQUE_FROM_PRIVATE(T, ...) std::unique_ptr<T>(detail::make_new_enabler<T>(__VA_ARGS__))
#else
#define NOTF_MAKE_SHARED_FROM_PRIVATE(T, ...) std::make_shared<detail::make_shared_enabler<T>>(__VA_ARGS__)
#define NOTF_MAKE_UNIQUE_FROM_PRIVATE(T, ...) std::make_unique<detail::make_shared_enabler<T>>(__VA_ARGS__)
#endif

// ================================================================================================================== //

/// Explicitly instantiate T using aggreate initialization.
/// Useful with `make_unique`, for example, whenever the aggregate constructor would not be able to be deduced
/// automatically. Usage:
///     std::make_uniqe<aggreate_adapter<X>>(foo, bar);
template<class T>
struct aggregate_adapter : public T {
    template<class... Args>
    aggregate_adapter(Args&&... args) : T{std::forward<Args>(args)...}
    {}
};

// ================================================================================================================== //

// TODO: replace old NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE with new and fancier NOTF_CREATE_SMART_FACTORIES

namespace detail {
template<class T>
struct MakeSharedAllocator : std::allocator<T> {

    template<class... Args>
    void construct(void* p, Args&&... args)
    {
        ::new (p) T(std::forward<Args>(args)...);
    }

    void destroy(T* p) { p->~T(); }
};
} // namespace detail

/// Creates two static factory methods inside a class to create an instance wrapped in a shared- or unique_ptr.
/// Works with classes with private constructor.
/// If you want to expose the factory methods, best create a public wrapper method with a concrete signature:
///
/// class Foo {
///     NOTF_CREATE_SMART_FACTORIES(Foo);   // smart factories should be private
///     Foo(int i);                         // private constructor
/// public:
///     static std::shared_ptr<Foo> create_shared(int i) { return _create_shared_Foo(i); }  // public factories
///     static std::unique_ptr<Foo> create_unique(int i) { return _create_unique_Foo(i); }
///
/// };
#define NOTF_CREATE_SMART_FACTORIES(X)                                                                       \
    template<class... Args>                                                                                  \
    static std::shared_ptr<X> NOTF_CONCAT(_create_shared_, X)(Args && ... args)                              \
    {                                                                                                        \
        return std::allocate_shared<X>(notf::detail::MakeSharedAllocator<X>(), std::forward<Args>(args)...); \
    }                                                                                                        \
    template<class... Args>                                                                                  \
    static std::unique_ptr<X> NOTF_CONCAT(_create_unique_, X)(Args && ... args)                              \
    {                                                                                                        \
        return std::unique_ptr<X>(new X(std::forward<Args>(args)...));                                       \
    }                                                                                                        \
    friend notf::detail::MakeSharedAllocator<X>

// ================================================================================================================== //

/// Like sizeof, but a returns the size of the type in bits not bytes.
template<class T>
inline constexpr size_t bitsizeof()
{
    return sizeof(T) * CHAR_BIT;
}

// ================================================================================================================== //

/// Helper struct to extract information about an unknown callable (like a lambda) at compile time.
/// From https://stackoverflow.com/a/47699977
/// Example:
///
///     auto lambda = [](const int& i) { return i == 1; };
///     using ft = function_traits<decltype(lambda)>;
///
///     static_assert(ft::has_return_type<bool>());         // success
///     static_assert(ft::has_arg_type<0, const int&>());   // success
///     // static_assert(ft::has_arg_type<0, float>());     // fail
///
///     auto okay_other = [](const int&) { return true; };
///     static_assert(ft::is_same<decltype(other)>());      // success
///
///     static_assert(ft::is_same<bool (&)(const int&)>()); // success
///
template<class Signature>
class function_traits : public function_traits<decltype(&Signature::operator())> {
    using impl_t = function_traits<decltype(&Signature::operator())>;

public:
    using return_type = typename impl_t::return_type;
    using arg_tuple = typename impl_t::arg_tuple;
    static constexpr auto arity = impl_t::arity;

    template<class T>
    static constexpr bool has_return_type()
    {
        return std::is_same_v<T, return_type>;
    }

    template<size_t index, class T>
    static constexpr bool has_arg_type()
    {
        return std::is_same_v<T, std::tuple_element_t<index, arg_tuple>>;
    }

    template<class T, class Indices = std::make_index_sequence<arity>>
    static constexpr bool is_same()
    {
        return all(function_traits<T>::arity == arity,                                    // same arity
                   std::is_same_v<typename function_traits<T>::return_type, return_type>, // same return type
                   _check_arg_types<T>(Indices{}));                                       // same argument types
    }

private:
    template<class T, std::size_t index>
    static constexpr bool _check_arg_type()
    {
        return std::is_same_v<typename function_traits<T>::template arg_type<index>,
                              typename impl_t::template arg_type<index>>;
    }
    template<class T, std::size_t... i>
    static constexpr bool _check_arg_types(std::index_sequence<i...>)
    {
        return (... && _check_arg_type<T, i>());
    }
};

template<class class_t, class return_t, class... Args>
struct function_traits<return_t (class_t::*)(Args...) const> {
    using return_type = return_t;
    using arg_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t i>
    using arg_type = std::tuple_element_t<i, arg_tuple>;
};

template<class return_t, class... Args>
struct function_traits<return_t (&)(Args...)> {
    using return_type = return_t;
    using arg_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);

    template<size_t i>
    using arg_type = std::tuple_element_t<i, arg_tuple>;
};

// ================================================================================================================== //

/// Extracts the last part of a pathname at compile time.
/// @param input     Path to investigate.
/// @param delimiter Delimiter used to separate path elements.
/// @return          Only the last part of the path, e.g. basename("/path/to/some/file.cpp", '/') would return
///                  "file.cpp".
constexpr const char* basename(const char* input, const char delimiter = '/')
{
    size_t last_occurrence = 0;
    for (size_t offset = 0; input[offset]; ++offset) {
        if (input[offset] == delimiter) {
            last_occurrence = offset + 1;
        }
    }
    return &input[last_occurrence];
}

NOTF_CLOSE_NAMESPACE
