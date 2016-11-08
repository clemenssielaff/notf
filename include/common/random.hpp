#pragma once

#include "randutils/randutils.hpp"

namespace notf {

using ushort = unsigned short;

/// @brief Convenience accessor to a randutils random engine.
///
/// Also saves the cost of re-creating a new random engine every time we want a random number.
inline decltype(randutils::default_rng()) & get_random_engine()
{
    static auto random_engine = randutils::default_rng();
    return random_engine;
}

} // namespace notf
