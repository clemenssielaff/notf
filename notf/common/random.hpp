#pragma once

#include "notf/common/geo/arithmetic.hpp"

#include <random>

#include "pcg_random.hpp"

NOTF_OPEN_NAMESPACE

// random number generator ========================================================================================== //

namespace detail {

pcg32& global_rng();

} // namespace detail

// random =========================================================================================================== //

/// Returns a single random scalar value.
template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
T random(const T min = 0, const T max = 1) {
    if constexpr (std::is_floating_point_v<T>) {
        return std::uniform_real_distribution<>(min, max)(detail::global_rng());
    } else {
        static_assert(std::is_integral_v<T>);
        return std::uniform_int_distribution<>(min, max)(detail::global_rng());
    }
}

/// Returns a random arithmetic value.
template<class T, class = std::enable_if_t<is_arithmetic_type_v<T>>>
T random(const typename T::element_t min = 0, const typename T::element_t max = 1) {
    T result;
    for (std::size_t dim = 0; dim < T::get_dimensions(); ++dim) {
        result.data[dim] = random<typename T::component_t>(min, max);
    }
    return result;
}

/// Returns a random boolean.
template<>
inline bool random<bool>(const bool, const bool) {
    return static_cast<bool>(random<int>());
}

/// Generates a random string from a pool characters.
/// If the pool is empty, the resulting string will be empty.
/// @param length   Length of the string (is not randomized).
/// @param pool     Pool of possible (not necessarily unique) characters.
template<class T, class = std::enable_if_t<std::is_same_v<T, std::string>>>
std::string random(const size_t length, const std::string_view pool) {
    const auto pool_size = pool.size();
    std::string result;
    if (length == 0 || pool_size == 0) {
        // do nothing
    } else if (pool_size == 1) {
        result.resize(length, pool[0]);
    } else {
        result.resize(length);
        auto distribution = std::uniform_int_distribution<>(0, pool_size - 1);
        auto& rng = detail::global_rng();
        for (size_t i = 0; i < length; ++i) {
            result[i] = distribution(rng);
        }
    }
    return result;
}
/// Returns a random angle in radians between -pi and pi.
template<class T = double>
std::enable_if_t<std::is_floating_point_v<T>, T> random_radian() {
    return random<T>(-pi<T>(), pi<T>());
}

/// Randomly calls one of the given lambdas.
/// All lambdas have to return void and take no arguments.
template<class... Lambdas>
constexpr void random_call(Lambdas... lambdas) {
    constexpr std::array<std::function<void()>, sizeof...(Lambdas)> all_lambdas{lambdas...};
    all_lambdas[random<size_t>(0, sizeof...(Lambdas) - 1)]();
}

NOTF_CLOSE_NAMESPACE
