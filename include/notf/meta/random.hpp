#pragma once

#include <string_view>

#include "./meta.hpp"
#include "./real.hpp"
#include "randutils/randutils.hpp"

NOTF_OPEN_META_NAMESPACE

// random generators ================================================================================================ //

/// Convenience accessor and cache of a randutils random engine.
decltype(randutils::default_rng())& get_random_engine();

/// Returns a random number.
/// @param from      Lowest possible number (default is 0)
/// @param to        Highest possible number (defaults to highest representable number)
template<class L, class R, typename T = std::common_type_t<L, R>>
T random_number(const L from, const R to)
{
    return get_random_engine().uniform(from, to);
}

/// Returns a random angle in radians between -pi and pi.
template<class T = double>
std::enable_if_t<std::is_floating_point_v<T>, T> random_radian()
{
    return random_number(-pi<T>(), pi<T>());
}

/// Generates a random string.
/// If none of the options is allowed, the resulting string will be empty.
/// @param length        Length of the string (is not randomized).
/// @param lowercase     Include lowercase letters?
/// @param uppercase     Include uppercase letters?
/// @param digits        Include digits?
std::string
random_string(const size_t length, const bool lowercase = true, const bool uppercase = true, const bool digits = true);

/// Generates a random string from a pool characters.
/// If the pool is empty, the resulting string will be empty.
/// @param length        Length of the string (is not randomized).
/// @param length        Pool of possible (not necessarily unique) characters.
std::string random_string(const size_t length, const std::string_view pool);

NOTF_CLOSE_META_NAMESPACE
