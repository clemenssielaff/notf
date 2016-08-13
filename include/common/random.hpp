#pragma once

#include "randutils/randutils.hpp"

namespace signal {

/// \brief Convenience accessor to a randutils random engine.
///
/// Also saves the cost of re-creating a new random engine every time we want a random number.
decltype(randutils::default_rng()) & random()
{
    static auto random_engine = randutils::default_rng();
    return random_engine;
}

} // namespace signal
