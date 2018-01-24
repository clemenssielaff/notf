#pragma once

#include <cassert>
#include <cstdint>

namespace notf {

struct Time {
    friend class Application; // to set the frequency at the start
    using Ticks = uint64_t;

    /** Ticks since the start of the Application. */
    Ticks ticks;

    Time() = default;

    Time(const Ticks ticks) : ticks(ticks) {}

    /** Seconds since the start of the Application. */
    double in_seconds() const
    {
        assert(s_frequency);
        return static_cast<double>(ticks) / static_cast<double>(s_frequency);
    }

    /** The current time. */
    static Time now();

    /** How much time passed since then.
     * Make sure that then <= now, otherwise returns zero.
     */
    static Time since(const Time& then);

    /** How much time will pass until then.
     * Make sure that then >= now, otherwise returns zero.
     */
    static Time until(const Time& then);

    /** Number of Ticks in a second. */
    static Ticks frequency() { return s_frequency; }

    /* Operators ******************************************************************************************************/

    Time& operator+=(const Time& rhs)
    {
        ticks += rhs.ticks;
        return *this;
    }

    Time& operator-=(const Time& rhs)
    {
        ticks = rhs.ticks <= ticks ? ticks - rhs.ticks : 0;
        return *this;
    }

    bool operator==(const Time& rhs) const { return ticks == rhs.ticks; }
    bool operator!=(const Time& rhs) const { return ticks != rhs.ticks; }
    bool operator>(const Time& rhs) const { return ticks > rhs.ticks; }
    bool operator>=(const Time& rhs) const { return ticks >= rhs.ticks; }
    bool operator<(const Time& rhs) const { return ticks < rhs.ticks; }
    bool operator<=(const Time& rhs) const { return ticks <= rhs.ticks; }
    explicit operator bool() const { return static_cast<bool>(ticks); }

    Time operator+(const Time& rhs) const { return {ticks + rhs.ticks}; }

    /** Time difference, is always positive. */
    Time operator-(const Time& rhs) const { return {ticks >= rhs.ticks ? ticks - rhs.ticks : rhs.ticks - ticks}; }

private: // static methods
    /** Initializes the static fields of the Time class at the beginning of the application. */
    static void initialize();

private: // fields
    /** Ticks on the system clock when the application was started (to keep the number of seconds low). */
    static Ticks s_zero;

    /** Ticks per second. */
    static Ticks s_frequency;
};

} // namespace notf
