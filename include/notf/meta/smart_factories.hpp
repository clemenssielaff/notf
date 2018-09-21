#pragma once

#include <memory>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// smart factories ================================================================================================== //

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
///     class Foo {
///         NOTF_CREATE_SMART_FACTORIES(Foo);   // smart factories should be private
///         Foo(int i);                         // private constructor
///     public:
///         static std::shared_ptr<Foo> create_shared(int i) { return _create_shared(i); }  // public factories
///         static std::unique_ptr<Foo> create_unique(int i) { return _create_unique(i); }
///     };
///
#define NOTF_CREATE_SMART_FACTORIES(X)                                                                       \
    template<class... Args>                                                                                  \
    static std::shared_ptr<X> _create_shared(Args&&... args)                                                 \
    {                                                                                                        \
        return std::allocate_shared<X>(notf::detail::MakeSharedAllocator<X>(), std::forward<Args>(args)...); \
    }                                                                                                        \
    template<class... Args>                                                                                  \
    static std::unique_ptr<X> _create_unique(Args&&... args)                                                 \
    {                                                                                                        \
        return std::unique_ptr<X>(new X(std::forward<Args>(args)...));                                       \
    }                                                                                                        \
    friend notf::detail::MakeSharedAllocator<X>

NOTF_CLOSE_NAMESPACE
