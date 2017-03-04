#pragma once

#include <string>

#include "common/float_utils.hpp"
#include "randutils/randutils.hpp"
#include "utils/sfinae.hpp"

namespace notf {

/** Convenience accessor and cache of a randutils random engine. */
decltype(randutils::default_rng()) & get_random_engine();

/** Returns a random number.
 * @param from      Lowest possible number (default is 0)
 * @param to        Highest possible number (defaults to highest representable number)
 */
template <typename Type>
Type random_number(const Type from = 0, const Type to = std::numeric_limits<Type>::max())
{
    return get_random_engine().uniform(from, to);
}

/** Returns a random angle in radians between -pi and pi. */
template <typename Real, ENABLE_IF_REAL(Real)>
Real random_radian()
{
    return random_number(-PI, PI);
}

/** Generates a random string.
 * If none of the options is allowed, the resulting string will be empty.
 * @param length        Length of the string (is not randomized).
 * @param lowercase     Include lowercase letters?
 * @param uppercase     Include uppercase letters?
 * @param digits        Include digits?
 */
std::string random_string(const size_t length,
                          const bool lowercase = true, const bool uppercase = true, const bool digits = true);

/** Generates a random string from a pool characters.
 * If the pool is empty, the resulting string will be empty.
 * @param length        Length of the string (is not randomized).
 * @param length        Pool of possible (not necessarily unique) characters.
 */
std::string random_string(const size_t length, const std::string& pool);

} // namespace notf
