#pragma once

#include <numeric>

#include "notf/meta/numeric.hpp"

NOTF_OPEN_NAMESPACE

// digits =========================================================================================================== //

/// Returns the nth digit from the right.
/// Digit #0 is the least significant digit.
template<std::size_t base = 10>
constexpr std::size_t get_digit(const std::size_t number, const uint digit) noexcept {
    static_assert(base > 1);
    return (number % exp(base, digit + 1)) / exp(base, digit);
}

/// Counts the digits in an integral number.
template<std::size_t base = 10>
constexpr std::size_t count_digits(std::size_t number) noexcept {
    static_assert(base > 1);
    std::size_t result = 1;
    while (number /= base) {
        ++result;
    }
    return result;
}

/// Tests if a given integer is a power of two.
template<class I, class = std::enable_if_t<std::is_integral_v<I>>>
constexpr bool is_power_of_two(const I number) noexcept {
    return 0 != number && 0 == (number & (number - 1));
}

// division ========================================================================================================= //

/// Tests if a value is even.
template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr bool is_even(const T& value) noexcept {
    return value % 2 == 0;
}

/// Tests if a value is odd.
template<class T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr bool is_odd(const T& value) noexcept {
    return value % 2 == 1;
}

/// Implements Python's integer modulo operation where negative values wrap around.
/// @param n     n % M
/// @param M     n % M
/// @return      n % M, while negative values are wrapped (for example -1%3=2).
template<class L, class R, class T = std::common_type_t<L, R>, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T wrap_mod(const L n, const R M) noexcept {
    return ((n % M) + M) % M;
}

/// Calculate the Greatest Common Divisor of two integers.
/// The GDC is the largest positive integer that divides each of the integers
/// @throws value_error If one or both numbers are zero.
template<class L, class R, typename T = std::common_type_t<L, R>, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T gcd(const L& lhs, const R& rhs) {
    T a = static_cast<T>(lhs);
    T b = static_cast<T>(rhs);
    if (a == 0 || b == 0) { NOTF_THROW(ValueError, "Cannot calculate the GCD of {} and {}", lhs, rhs); }

    while (true) {
        if (a == 0) { return b; }
        b %= a;

        if (b == 0) { return a; }
        a %= b;
    }
}

/// Calculates the Least Common Multiple of two or more integers.
/// @throws value_error If any given integer is zero.
template<class L, class R, class T = std::common_type_t<L, R>, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr T lcm(const L& lhs, const R& rhs) {
    return abs(lhs * rhs) / gcd(lhs, rhs);
}
template<class L, class R, class... Args>
constexpr auto lcm(const L& lhs, const R& rhs, Args&&... args) {
    return lcm(lhs, lcm(rhs, std::forward<Args>(args)...));
}
template<class Iterable>
constexpr auto lcm(const Iterable& numbers) {
    return std::accumulate(std::begin(numbers), std::end(numbers), 1,
                           [](const auto& a, const auto& b) { return abs(a * b) / gcd(a, b); });
}

// pascal triangle ================================================================================================== //

constexpr ulong binomial(const ulong n, const ulong k) {
    if (k > n) { NOTF_THROW(LogicError, "Cannot calculate binomial coefficient with k > n"); }
    ulong result = 1;
    for (ulong itr = 1; itr <= k; ++itr) {
        result = (result * (n + 1 - itr)) / itr;
    }
    return result;
}

/// https://en.wikipedia.org/wiki/Pascal%27s_triangle
template<ulong N, class T = ulong, class = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr std::array<T, N + 1> pascal_triangle_row() {
    T result[N + 1]{};
    result[0] = 1;
    for (ulong column = 1; column <= N; ++column) {
        result[column] = (result[column - 1] * (N + 1 - column)) / column;
    }
    return to_array(result);
}

NOTF_CLOSE_NAMESPACE
