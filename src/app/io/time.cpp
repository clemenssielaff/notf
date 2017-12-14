#include "app/io/time.hpp"

#include <cstdint>
#include <type_traits>

#include "app/core/glfw.hpp"
#include "common/log.hpp"

namespace notf {

Time::Ticks Time::s_zero      = {0};
Time::Ticks Time::s_frequency = {0};

Time Time::now() { return {glfwGetTimerValue() - s_zero}; }

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

void Time::initialize()
{
    s_zero      = glfwGetTimerValue();
    s_frequency = glfwGetTimerFrequency();
    log_info << "Setting Time::frequency to: " << s_frequency;
}

static_assert(std::is_pod<Time>::value, "This compiler does not recognize notf::Time as a POD.");

} // namespace notf