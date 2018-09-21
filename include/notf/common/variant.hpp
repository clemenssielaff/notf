#pragma once

#include <variant>

#include "./common.hpp"

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
template<typename... Ts>
using tuple_to_variant_t = typename tuple_to_variant<Ts...>::type;

NOTF_CLOSE_NAMESPACE
