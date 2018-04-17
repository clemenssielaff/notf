#pragma once

#include <chrono>
#include <condition_variable>

#include <chrono>
#include <cstdint>
#include <functional>
#include <ratio>

#include "app/forwards.hpp"
#include "common/mutex.hpp"
#include "common/thread.hpp"

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

/// A single thread running 0-n Timer instances used to trigger timed events like animations.
class AnimationManager {

    // types ---------------------------------------------------------------------------------------------------------//
private:
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

public:
    class Timer {

        Timer(const duration_t duration, const bool is_repeating = false)
            : m_interval(duration), m_is_repeating(is_repeating)
        {}

        void start() { m_next_time = std::chrono::time_point_cast<duration_t>(clock_t::now() + m_interval); }

        void reset()
        {
            if (m_is_repeating) {
                m_next_time += m_interval;
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Time when the timer fires next.
        time_point_t m_next_time;

        /// Time between firing.
        duration_t m_interval;

        /// Whether the timer is repeating or not.
        bool m_is_repeating;
    };
};

NOTF_CLOSE_NAMESPACE
