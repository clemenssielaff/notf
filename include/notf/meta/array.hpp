#pragma once

#include <array>

#include <notf/meta/types.hpp>

NOTF_OPEN_NAMESPACE

// make array ======================================================================================================= //

namespace detail {
template<class T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array_impl(T (&a)[N], std::index_sequence<I...>) {
    return {{a[I]...}};
}
} // namespace detail

/// Creates a std::array from the built-in array a.
/// The elements of the std::array are copy-initialized from the corresponding element of a.
/// From https://en.cppreference.com/w/cpp/experimental/to_array
/// @param a    Built-in array.
/// @returns    std::array
template<class T, std::size_t N>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N]) {
    return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

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
