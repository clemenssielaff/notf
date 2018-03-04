#pragma once

#include <string>

#include "common/float.hpp"
#include "common/meta.hpp"
#include "randutils/randutils.hpp"

NOTF_OPEN_NAMESPACE

/** Convenience accessor and cache of a randutils random engine. */
decltype(randutils::default_rng())& get_random_engine();

/** Returns a random number.
 * @param from      Lowest possible number (default is 0)
 * @param to        Highest possible number (defaults to highest representable number)
 */
template<typename Type>
Type random_number(const Type from, const Type to)
{
    return get_random_engine().uniform(from, to);
}

/** Returns a random angle in radians between -pi and pi. */
template<typename Real, typename = std::enable_if_t<std::is_floating_point<Real>::value>(Real)>
Real random_radian()
{
    return random_number(-pi<Real>(), pi<Real>());
}

/** Generates a random string.
 * If none of the options is allowed, the resulting string will be empty.
 * @param length        Length of the string (is not randomized).
 * @param lowercase     Include lowercase letters?
 * @param uppercase     Include uppercase letters?
 * @param digits        Include digits?
 */
std::string
random_string(const size_t length, const bool lowercase = true, const bool uppercase = true, const bool digits = true);

/** Generates a random string from a pool characters.
 * If the pool is empty, the resulting string will be empty.
 * @param length        Length of the string (is not randomized).
 * @param length        Pool of possible (not necessarily unique) characters.
 */
std::string random_string(const size_t length, const std::string& pool);

NOTF_CLOSE_NAMESPACE
