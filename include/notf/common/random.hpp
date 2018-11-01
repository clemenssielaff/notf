#pragma once

#include "randutils/randutils.hpp"

#include "notf/common/arithmetic.hpp"

NOTF_OPEN_NAMESPACE

// random =========================================================================================================== //

/// Returns a random scalar value.
template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
T random(const T min = 0, const T max = 1)
{
    return randutils::global_rng().uniform(min, max);
}

/// Returns a random arithmetic value.
template<class T>
std::enable_if_t<detail::is_arithmetic_type<T>, T>
random(const typename T::element_t min = 0, const typename T::element_t max = 1)
{
    T result;
    for (size_t dim = 0; dim < T::get_dimensions(); ++dim) {
        result.data[dim] = random<typename T::component_t>(min, max);
    }
    return result;
}

/// Generates a random string from a pool characters.
/// If the pool is empty, the resulting string will be empty.
/// @param length   Length of the string (is not randomized).
/// @param pool     Pool of possible (not necessarily unique) characters.
template<class T, class = std::enable_if_t<std::is_same_v<T, std::string>>>
T random(const size_t length, const std::string_view pool)
{
    if (pool.empty()) { return {}; }
    std::string result(length, pool[0]);
    auto random_engine = randutils::global_rng();
    for (size_t i = 0; i < length; ++i) {
        result[i] = *random_engine.choose(std::begin(pool), std::end(pool));
    }
    return result;
}
/// Returns a random angle in radians between -pi and pi.
template<class T = double>
std::enable_if_t<std::is_floating_point_v<T>, T> random_radian()
{
    return random<T>(-pi<T>(), pi<T>());
}

NOTF_CLOSE_NAMESPACE
