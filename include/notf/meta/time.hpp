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

/// Point in time.
using timepoint_t = std::chrono::time_point<clock_t, duration_t>;

static_assert (std::is_trivially_copyable_v<duration_t>);
static_assert (std::is_trivially_copyable_v<timepoint_t>);

// functions ======================================================================================================== //

/// What time is it right now?
inline timepoint_t now() { return std::chrono::time_point_cast<duration_t>(clock_t::now()); }

/// Duration from seconds.
template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
inline constexpr auto to_seconds(T seconds) {
    return duration_t(static_cast<duration_t::rep>(static_cast<T>(duration_t::period::den) * seconds));
}

/// Duration from minutes.
template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
inline constexpr auto to_minutes(T minutes) {
    return duration_t(static_cast<duration_t::rep>(static_cast<T>(duration_t::period::den) * minutes * T(60)));
}

// fps literal ====================================================================================================== //

NOTF_OPEN_LITERALS_NAMESPACE

using namespace std::chrono_literals;

/// User-defined "_fps" literal.
constexpr duration_t operator""_fps(unsigned long long fps) { return duration_t(duration_t::period::den / fps); }
constexpr duration_t operator""_fps(long double fps) {
    return duration_t(static_cast<duration_t::rep>(duration_t::period::den / fps));
}

NOTF_CLOSE_LITERALS_NAMESPACE
NOTF_CLOSE_NAMESPACE
