#include "catch.hpp"

#include <atomic>
#include <iostream>
#include <thread>

#include "app/timer_manager.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE;

// ================================================================================================================== //

SCENARIO("Timer can be used to schedule asyncronous callbacks", "[app], [timer]")
{
    using namespace ::notf::literals;

    static_assert(10_fps == 100ms, "custom literal '_fps' does not work");
    static_assert(100_fps == 10ms, "custom literal '_fps' does not work");

    SECTION("a single-shot Timer schedules a callback for the future")
    {
        std::atomic_size_t counter(0);
        OneShotTimer::create(50ms, [&] { ++counter; });
        std::this_thread::sleep_for(55ms);
        REQUIRE(counter == 1);
    }

    SECTION("a single-shot Timer is called immediately if the time has already passed")
    {
        std::atomic_size_t counter(0);
        OneShotTimer::create(0ms, [&] { ++counter; });
        REQUIRE(counter == 1);
    }

    SECTION("a repeating Timer can be used for a steady fps")
    {
        std::atomic_size_t counter(0);
        IntervalTimerPtr fps_timer = IntervalTimer::create([&] { ++counter; });
        fps_timer->start(1000_fps);
        std::this_thread::sleep_for(50ms);
        fps_timer->stop();
        const size_t result = counter.load();
        REQUIRE((48 <= result && result <= 52));
    }

    SECTION("Timer can be set to only repeat a certain amount of times")
    {
        std::atomic_size_t counter(0);
        IntervalTimerPtr limited_timer = IntervalTimer::create([&] { ++counter; });
        limited_timer->start(1000_fps, 42);
        std::this_thread::sleep_for(50ms);
        const size_t result = counter.load();
        REQUIRE(result == 42);
    }

    SECTION("Variable Timer can reschedule with a different interval each time")
    {
        std::atomic_size_t counter(0);
        VariableTimerPtr variable_timer = VariableTimer::create([&] { ++counter; });
        {
            int wait_time = 256;
            variable_timer->start(
                [wait_time]() mutable {
                    wait_time /= 2;
                    return std::chrono::milliseconds(wait_time);
                },
                8);
            ;
        }
        std::this_thread::sleep_for(
            std::chrono::milliseconds(128 + 64 + 32 + 16 + 8 + 4 + 2 + 1 /* for good measure*/ + 1));
        REQUIRE(counter.load() == 8);
    }
}
