#pragma once

#include <cassert>
#include <cstdint>

namespace notf {

//====================================================================================================================//

/// Time class using GLFW's representation of `ticks`.
class Time {
    friend class Application; // to set the frequency at the start

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// Smallest unit of time known to the application.
    using Ticks = uint64_t;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Default constructor.
    Time() = default;

    /// Copy constructor.
    Time(const Ticks ticks) : ticks(ticks) {}

    /// Invalid time.
    static Time invalid() { return Time(0); }

    /// Checks if the time value is valid.
    bool is_valid() const { return static_cast<bool>(ticks); }

    /// Checks if the time value is invalid.
    bool is_invalid() const { return !is_valid(); }

    /// Seconds since the start of the Application.
    double in_seconds() const
    {
        assert(s_frequency);
        return static_cast<double>(ticks) / static_cast<double>(s_frequency);
    }

    /// The current time.
    static Time now();

    /// How much time passed since then.
    /// Make sure that then <= now, otherwise returns zero.
    static Time since(const Time& then);

    /// How much time will pass until then.
    /// Make sure that then >= now, otherwise returns zero.
    static Time until(const Time& then);

    /// Number of Ticks in a second.
    static Ticks frequency() { return s_frequency; }

    /// Addition operator.
    Time operator+(const Time& rhs) const { return {ticks + rhs.ticks}; }

    /// In-place addition operator.
    Time& operator+=(const Time& rhs)
    {
        ticks += rhs.ticks;
        return *this;
    }

    /// In-place subtraction operator, the result is never less than zero.
    /// Use `operator -` to calculate the (always positive) time difference.
    Time& operator-=(const Time& rhs)
    {
        ticks = rhs.ticks <= ticks ? ticks - rhs.ticks : 0;
        return *this;
    }

    /// Time difference, is always positive.
    Time operator-(const Time& rhs) const { return {ticks >= rhs.ticks ? ticks - rhs.ticks : rhs.ticks - ticks}; }

    /// Equality operator.
    bool operator==(const Time& rhs) const { return ticks == rhs.ticks; }

    /// Inequality operator.
    bool operator!=(const Time& rhs) const { return ticks != rhs.ticks; }

    /// Checks if this point is time is later than the other.
    bool operator>(const Time& rhs) const { return ticks > rhs.ticks; }

    /// Checks if this point is time is later or equal than the other.
    bool operator>=(const Time& rhs) const { return ticks >= rhs.ticks; }

    /// Checks if this point is time is earlier than the other.
    bool operator<(const Time& rhs) const { return ticks < rhs.ticks; }

    /// Checks if this point is time is earlier or equal than the other.
    bool operator<=(const Time& rhs) const { return ticks <= rhs.ticks; }

    /// Checks if the time value is valid.
    explicit operator bool() const { return is_valid(); }

    /// Checks if the time value is invalid.
    bool operator!() const { return !is_valid(); }

private: // for Application
    /// Initializes the static fields of the Time class at the beginning of the application.
    static void initialize();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Ticks since the start of the Application.
    Ticks ticks;

    /// Ticks on the system clock when the application was started (to keep the number of seconds low).
    static Ticks s_zero;

    /// Ticks per second.
    static Ticks s_frequency;
};

} // namespace notf
