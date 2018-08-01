#pragma once

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// C++17 helper struct to use with `std::visit`.
/// From: https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

NOTF_CLOSE_NAMESPACE
