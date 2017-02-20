#pragma once

#include <assert.h>
#include <cstdint>

namespace notf {

struct Time {
    friend class Application; // to set the frequency at the start
    using Ticks = uint64_t;

    /** Ticks since the start of the Application. */
    Ticks ticks;

    /** Seconds since the start of the Application. */
    double in_seconds() const
    {
        assert(frequency);
        return static_cast<double>(ticks) / static_cast<double>(frequency);
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

    /* Operators ******************************************************************************************************/

    bool operator==(const Time& rhs) const { return ticks == rhs.ticks; }
    bool operator!=(const Time& rhs) const { return ticks != rhs.ticks; }
    bool operator>(const Time& rhs) const { return ticks > rhs.ticks; }
    bool operator>=(const Time& rhs) const { return ticks >= rhs.ticks; }
    bool operator<(const Time& rhs) const { return ticks < rhs.ticks; }
    bool operator<=(const Time& rhs) const { return ticks <= rhs.ticks; }
    explicit operator bool() const { return static_cast<bool>(ticks); }

    Time operator+(const Time& rhs) const { return {ticks + rhs.ticks}; }

    /** Time difference, is always positive. */
    Time operator-(const Time& rhs) const
    {
        return {ticks >= rhs.ticks ? ticks - rhs.ticks : rhs.ticks - ticks};
    }

//private: // static methods
public: // static methods // TODO: revert to private after the font test is done
    /** Set the global frequency value. */
    static void set_frequency(Ticks ticks);

private: // fields
    /** Ticks per second. */
    static Ticks frequency;
};

} // namespace notf
