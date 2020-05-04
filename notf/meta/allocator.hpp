#pragma once

#include "notf/meta/fwd.hpp"

#include <memory>

NOTF_OPEN_NAMESPACE

// default initializing allocator ================================================================================== //

/// Allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
/// From https://stackoverflow.com/a/21028912
template<class T, class SuperT = std::allocator<T>>
class DefaultInitAllocator : public SuperT {

    // types ----------------------------------------------------------------------------------- //
private:
    using allocator_traits = std::allocator_traits<SuperT>;

    // methods --------------------------------------------------------------------------------- //
public:
    template<class U>
    struct rebind {
        using other = DefaultInitAllocator<U, typename allocator_traits::template rebind_alloc<U>>;
    };

    template<class U>
    void construct(U* ptr) noexcept(std::is_nothrow_default_constructible_v<U>) {
        ::new (static_cast<void*>(ptr)) U;
    }
    template<class U, class... Args>
    void construct(U* ptr, Args&&... args) {
        allocator_traits::construct(static_cast<SuperT&>(*this), ptr, std::forward<Args>(args)...);
    }
};

NOTF_CLOSE_NAMESPACE