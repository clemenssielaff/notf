#pragma once

#include <variant>

#include "notf/meta/macros.hpp"
#include "notf/meta/tuple.hpp"

NOTF_OPEN_NAMESPACE

// variant ========================================================================================================== //

/// Trait helping to create a nice std::variant visitor.
/// Use like this:
///
///     std::visit(overloaded {
///         [](auto arg) { std::cout << arg << ' '; },
///         [](double arg) { std::cout << std::fixed << arg << ' '; },
///         [](const std::string& arg) { std::cout << std::quoted(arg) << ' '; },
///     }, some_variant);
///
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

/// Creates a variant that is able to hold all types in a given tuple.
/// The first type in the tuple will be the default type of the variant.
template<class Tuple>
struct tuple_to_variant;
template<typename... Ts>
struct tuple_to_variant<std::tuple<Ts...>> {
    using type = std::variant<Ts...>;
};
template<typename Tuple>
using tuple_to_variant_t = typename tuple_to_variant<Tuple>::type;

/// Creates a tuple that is able to hold all types in a given variant.
template<class Variant>
struct variant_to_tuple;
template<typename... Ts>
struct variant_to_tuple<std::variant<Ts...>> {
    using type = std::tuple<Ts...>;
};
template<typename Variant>
using variant_to_tuple_t = typename variant_to_tuple<Variant>::type;

/// Finds and returns the first index of the given type in the Variant.
/// @throws std::overflow_error To make this a non-constexpr on failure.
template<class T, class Variant, size_t I = 0>
NOTF_UNUSED static constexpr size_t get_first_variant_index() {
    if constexpr (I == std::variant_size_v<Variant>) {
        throw 0;
    } else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>, T>) {
        return I;
    } else {
        return get_first_variant_index<T, Variant, I + 1>();
    }
}

/// Checks if a given type is part of the Variant.
template<class T, class Variant, size_t I = 0>
NOTF_UNUSED static constexpr bool is_one_of_variant() noexcept {
    if constexpr (I == std::variant_size_v<Variant>) {
        return false;
    } else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>, T>) {
        return true;
    } else {
        return is_one_of_variant<T, Variant, I + 1>();
    }
}

/// Checks if a variant has every type only once.
template<class Variant>
NOTF_UNUSED static constexpr bool has_variant_unique_types() noexcept {
    return std::tuple_size_v<make_tuple_unique_t<variant_to_tuple_t<Variant>>> == std::variant_size_v<Variant>;
}

// visit at ========================================================================================================= //

/// @{
/// Applies a given function to a single element of the variant, identified by index.
/// If you expect a return type from the function, you'll have to manually declare it as a template argument, because we
/// cannot infer the tuple element type from a runtime index.
/// Implemented after https://stackoverflow.com/a/47385405
/// @param variant  Variant to visit over.
/// @param index    Index to visit at.
/// @param function Function to execute. Must take the variant element type as first argument.
/// @param args     (optional) Additional arguments passed bound to the 2nd to nth parameter of function.
template<class Function, class... Ts, class... Args, size_t Size = sizeof...(Ts)>
constexpr void visit_at(const std::variant<Ts...>& variant, size_t index, Function function, Args&&... args) noexcept(
    noexcept(detail::visit_at_impl<Size>::visit(variant, index, function, std::forward<Args>(args)...))) {
    detail::visit_at_impl<Size>::visit(variant, index, function, std::forward<Args>(args)...);
}
template<class Result, class... Ts, class Function, class... Args, size_t Size = sizeof...(Ts)>
constexpr Result
visit_at(const std::variant<Ts...>& variant, size_t index, Function function, Args&&... args) noexcept(noexcept(
    detail::visit_at_impl<Size>::template visit<Result>(variant, index, function, std::forward<Args>(args)...))) {
    return detail::visit_at_impl<Size>::template visit<Result>(variant, index, function, std::forward<Args>(args)...);
}
// @}

NOTF_CLOSE_NAMESPACE
