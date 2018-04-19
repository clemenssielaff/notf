#include "catch.hpp"

#include <atomic>
#include <iostream>
#include <thread>

#include "app/timer_manager.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

//====================================================================================================================//

SCENARIO("Timer can be used to schedule asyncronous callbacks", "[app], [timer]")
{
    using namespace std::chrono_literals;
    using namespace ::notf::literals;

    static_assert(10_fps == 100ms, "custom literal '_fps' does not work");
    static_assert(100_fps == 10ms, "custom literal '_fps' does not work");

    SECTION("a repeating Timer can be used for a steady fps")
    {
        std::atomic_size_t counter(0);
        TimerPtr fps_timer = Timer::create([&] { ++counter; });
        fps_timer->start(120_fps);
        std::this_thread::sleep_for(250ms);
        fps_timer->stop();
        REQUIRE(counter == 30);
    }
}
