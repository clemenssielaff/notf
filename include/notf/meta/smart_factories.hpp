#pragma once

#include <memory>

#include "./config.hpp"

NOTF_OPEN_NAMESPACE

// smart factories ================================================================================================== //

#define NOTF_CREATE_SMART_FACTORIES(X)                                            \
    template<class... Args>                                                       \
    static std::shared_ptr<X> _create_shared(Args&&... args)                      \
    {                                                                             \
        struct MakeSharedEnabler : public X {                                     \
            MakeSharedEnabler(Args&&... args) : X(std::forward<Args>(args)...) {} \
        };                                                                        \
        return std::make_shared<MakeSharedEnabler>(std::forward<Args>(args)...);  \
    }                                                                             \
    template<class... Args>                                                       \
    static std::unique_ptr<X> _create_unique(Args&&... args)                      \
    {                                                                             \
        return std::unique_ptr<X>(new X(std::forward<Args>(args)...));            \
    }                                                                             \
    template<class...>                                                            \
    friend std::unique_ptr<X> _create_unique(...)

NOTF_CLOSE_NAMESPACE
