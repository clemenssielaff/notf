#pragma once

#include <memory>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// smart factories ================================================================================================== //

#define NOTF_CREATE_SMART_FACTORIES(X)                                            \
    template<class... Args>                                                       \
    static std::shared_ptr<X> _create_shared(Args&&... args) {                    \
        struct MakeSharedEnabler : public X {                                     \
            MakeSharedEnabler(Args&&... args) : X(std::forward<Args>(args)...) {} \
        };                                                                        \
        return std::make_shared<MakeSharedEnabler>(std::forward<Args>(args)...);  \
    }                                                                             \
    template<class... Args>                                                       \
    static std::unique_ptr<X> _create_unique(Args&&... args) {                    \
        return std::unique_ptr<X>(new X(std::forward<Args>(args)...));            \
    }                                                                             \
    template<class...>                                                            \
    friend std::unique_ptr<X> _create_unique(...)

/// Explicitly instantiate T using aggregate initialization.
/// Useful with `make_unique`, for example, whenever the aggregate constructor would not be able to be deduced
/// automatically. Usage:
///     std::make_unique<aggregate_initialized<X>>(foo, bar);
template<class T, class... Args>
auto make_unique_aggregate(Args&&... args) {
    struct aggregate_initialized : public T {
        aggregate_initialized(Args&&... args) : T{std::forward<Args>(args)...} {}
    };
    return std::unique_ptr<T>(
        static_cast<T*>(std::make_unique<aggregate_initialized>(std::forward<Args>(args)...).release()));
}

NOTF_CLOSE_NAMESPACE
