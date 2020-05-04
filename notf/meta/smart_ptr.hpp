#pragma once

#include "notf/meta/config.hpp"

#include <memory>

NOTF_OPEN_NAMESPACE

// smart factories ================================================================================================== //

#define NOTF_CREATE_SMART_FACTORIES(X)                                                                    \
    template<class... SmartFactoryArgs>                                                                   \
    static std::shared_ptr<X> _create_shared(SmartFactoryArgs&&... args) {                                \
        struct MakeSharedEnabler : public X {                                                             \
            MakeSharedEnabler(SmartFactoryArgs&&... args) : X(std::forward<SmartFactoryArgs>(args)...) {} \
        };                                                                                                \
        return std::make_shared<MakeSharedEnabler>(std::forward<SmartFactoryArgs>(args)...);              \
    }                                                                                                     \
    template<class... SmartFactoryArgs>                                                                   \
    static std::unique_ptr<X> _create_unique(SmartFactoryArgs&&... args) {                                \
        return std::unique_ptr<X>(new X(std::forward<SmartFactoryArgs>(args)...));                        \
    }                                                                                                     \
    template<class...>                                                                                    \
    friend std::unique_ptr<X> _create_unique(...)

// make unique aggregate ================================================================================================ //

/// Explicitly instantiate T using aggregate initialization.
/// Useful with `make_unique`, for example, whenever the aggregate constructor would not be able to be deduced
/// automatically. Usage:
///     std::make_unique<aggregate_initialized<X>>(foo, bar);
template<class T, class... SmartFactoryArgs>
auto make_unique_aggregate(SmartFactoryArgs&&... args) {
    struct aggregate_initialized : public T {
        aggregate_initialized(SmartFactoryArgs&&... args) : T{std::forward<SmartFactoryArgs>(args)...} {}
    };
    return std::unique_ptr<T>(
        static_cast<T*>(std::make_unique<aggregate_initialized>(std::forward<SmartFactoryArgs>(args)...).release()));
}

// make shared array ================================================================================================ //

/// So it turns out using shared_ptr with c-style bounded arrays is kinda broken...?
/// The standard says that in c++17 it should be fine to call `std::shared_ptr<T[]>(new T[size])`, but apparently is 
/// just not implemented in libc++. Not broken, it's just not there.
/// The standard also sais that before c++17, you could use `std::unique_ptr<T[]>(new T[size])` to safely initialize the
/// pointer and then pass it into the `shared_ptr`, but apparently that should no longer work in c++17? Well, it doesn't
/// work in libc++, so I guess that option is out anyway.
/// In the end, this solution works both for GCC and Clang, using libstdc++ and libc++ respectively.
/// Ideally, you'd want to be able to say `std::make_shared<T[]>(72)` to create a bounded array of 72 `T`s, which 
/// hopefully will be in c++20, unless of course the standard library writers just choose to ignore it.
/// In order to make the code forward- compatible, use this function when creating a shared_ptr to a bounded array.
/// @param size     Number of elements in the array.
/// @returns        New `shared_ptr` managing a bounded array of T with size `size`.
template<class T>
auto make_shared_array(const std::size_t size) {
#if __cplusplus > 201703L // c++20
    return std::make_shared<T[]>(size);
#else
    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
#endif
}

NOTF_CLOSE_NAMESPACE
