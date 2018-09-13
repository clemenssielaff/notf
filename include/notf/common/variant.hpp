#pragma once

#include <variant>

#include "./common.hpp"

NOTF_OPEN_COMMON_NAMESPACE

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

NOTF_CLOSE_COMMON_NAMESPACE
