#include "catch.hpp"

#include "test/app.hpp"

#include "notf/app/application.hpp"
#include "notf/app/timer_pool.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("TimerPool", "[app][timer_pool]") {
    SECTION("without an application") { REQUIRE_THROWS_AS(*TheTimerPool(), SingletonError); }

    SECTION("with a properly initialized application") { TheApplication app(test_app_arguments()); }
}
