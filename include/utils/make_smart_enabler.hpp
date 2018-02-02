#pragma once

#include <utility>

namespace notf {

/// Helper struct to allow `std::make_shared` to work with protected constructors.
/// from:
///     https://stackoverflow.com/a/8147213/3444217 and https://stackoverflow.com/a/25069711/3444217
template<typename T>
struct make_shared_enabler : public T {
    template<typename... Args>
    make_shared_enabler(Args&&... args) : T(std::forward<Args>(args)...)
    {}
};

} // namespace notf
