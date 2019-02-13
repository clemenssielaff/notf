#pragma once

#include <array>

#include <notf/meta/types.hpp>

NOTF_OPEN_NAMESPACE

// make array of ==================================================================================================== //

namespace detail {

template<class T, size_t... Indices>
constexpr std::array<T, sizeof...(Indices)> make_array_of_impl(const T& value, std::index_sequence<Indices...>) {
    return {identity_func<T, Indices>(value)...};
}

} // namespace detail

/// Produces an array of given size with each element set to the given value.
/// @param value    Value to initialize every element of the array with.
template<size_t Size, class T>
constexpr std::array<T, Size> make_array_of(const T& value) {
    using Indices = std::make_index_sequence<Size>;
    return detail::make_array_of_impl(value, Indices{});
}

NOTF_CLOSE_NAMESPACE
