#pragma once

#include <optional>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// inspection ======================================================================================================= //

/// Template struct to test whether a given type is an optional or not.
template<class T>
struct is_optional : std::false_type {};
template<class T>
struct is_optional<std::optional<T>> : std::true_type {};
template<class T>
inline constexpr const bool is_optional_v = is_optional<T>::value;

NOTF_CLOSE_NAMESPACE
