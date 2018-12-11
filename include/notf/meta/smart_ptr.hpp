#pragma once

#include <memory>

#include "notf/meta/config.hpp"

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

NOTF_CLOSE_NAMESPACE
