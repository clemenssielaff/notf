#include "common/time.hpp"

#include <cstdint>
#include <type_traits>

#include "common/log.hpp"
#include "core/glfw_wrapper.hpp"

namespace notf {

Time::Ticks Time::frequency = {0};

Time Time::now()
{
    return {glfwGetTimerValue()};
}

Time Time::since(const Time& then)
{
    const Time time_now = now();
    if (time_now < then) {
        log_warning << "Cannot calculate time since a point in the future.";
        return {0};
    }
    return time_now - then;
}

Time Time::until(const Time& then)
{
    const Time time_now = now();
    if (time_now > then) {
        log_warning << "Cannot calculate time until a point in the past.";
        return {0};
    }
    return then - time_now;
}

void Time::set_frequency(Ticks ticks)
{
    assert(frequency == 0); // should only be called once
    assert(ticks != 0);
    frequency = ticks;
}

static_assert(std::is_pod<Time>::value, "This compiler does not recognize notf::Time as a POD.");

} // namespace notf