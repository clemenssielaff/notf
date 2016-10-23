#pragma once

namespace notf {

/// @brief Implements Python's modulo operation where negative values wrap around.
///
/// @param n    n % M
/// @param M    n % M
///
/// @return n % M, while negative values are wrapped (for example -1%3=2).
inline int wrap_mod(int n, int M)
{
    return ((n % M) + M) % M;
}

} // namespace notf
