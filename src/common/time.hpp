#pragma once

#include <chrono>

#include "common/meta.hpp"

NOTF_OPEN_NAMESPACE

/// Most precise but steady clock.
using clock_t = std::conditional_t<std::chrono::high_resolution_clock::is_steady, // if it is steady
                                   std::chrono::high_resolution_clock,            // use the high-resolution clock,
                                   std::chrono::steady_clock>;                    // otherwise use the steady clock

/// The famous "flicks" duration type, described in length at:
///     https://github.com/OculusVR/Flicks/blob/master/flicks.h
/// BSD License:
///     https://github.com/OculusVR/Flicks/blob/master/LICENSE
using duration_t = std::chrono::duration<std::chrono::nanoseconds::rep, std::ratio<1, 705600000>>;

/// Point in time.
using time_point_t = std::chrono::time_point<clock_t, duration_t>;

// ================================================================================================================== //

namespace literals {

using namespace std::chrono_literals;

/// User-defined "_fps" literal.
constexpr auto operator""_fps(unsigned long long fps) { return duration_t{duration_t::period::den / fps}; }
constexpr auto operator""_fps(long double fps)
{
    return std::chrono::duration<long double, duration_t::period>(duration_t::period::den / fps);
}

} // namespace literals

NOTF_CLOSE_NAMESPACE
