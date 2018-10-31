#pragma once

#include <chrono>

#include "notf/meta/config.hpp"

NOTF_OPEN_NAMESPACE

// time types ======================================================================================================= //

/// Most precise but steady clock.
using clock_t = std::conditional_t<std::chrono::high_resolution_clock::is_steady, // if it is steady
                                   std::chrono::high_resolution_clock,            // use the high-resolution clock,
                                   std::chrono::steady_clock>;                    // otherwise use the steady clock

/// The famous "flicks" duration type, described in length at:
///     https://github.com/OculusVR/Flicks/blob/master/flicks.h
/// BSD License:
///     https://github.com/OculusVR/Flicks/blob/master/LICENSE
using duration_t = std::chrono::duration<std::chrono::nanoseconds::rep, std::ratio<1, 705600000>>;

/// We use nanoseconds as base units of time.
/// NoTF used to use FB's fancy "flicks" units, until it became apparent that they don't work properly with
/// conditional_variable::wait_until (caused 100% CPU utilization in the TimerManager, when compiled with GCC 7.2.0)
/// If you want to check them out, look at this file (common/time.hpp) in revision:
///     a5508cd0111ef638c9c710c66abee9ca9084dd3a
// using duration_t = std::chrono::nanoseconds;

/// Point in time.
using timepoint_t = std::chrono::time_point<clock_t, duration_t>;

NOTF_CLOSE_NAMESPACE

// fps literal ====================================================================================================== //

NOTF_OPEN_LITERALS_NAMESPACE
NOTF_USING_NAMESPACE;

using namespace std::chrono_literals;

/// User-defined "_fps" literal.
constexpr duration_t operator""_fps(unsigned long long fps) { return duration_t(duration_t::period::den / fps); }
constexpr duration_t operator""_fps(long double fps)
{
    return duration_t(static_cast<duration_t::rep>(duration_t::period::den / fps));
}

NOTF_CLOSE_LITERALS_NAMESPACE
