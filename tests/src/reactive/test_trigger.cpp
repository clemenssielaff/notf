#include "catch2/catch.hpp"

#include "notf/reactive/trigger.hpp"

#include "test_reactive_utils.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("trigger", "[reactive][trigger]") {
    SECTION("Trigger<T>") {
        auto publisher = TestPublisher();
        int counter = 0;
        auto pipe = publisher | Trigger([&](int value) { counter += value; });

        REQUIRE(counter == 0);
        publisher->publish(45);
        REQUIRE(counter == 45);
    }
    SECTION("Trigger<None>") {
        auto publisher = DefaultPublisher<None>();
        int counter = 0;
        auto pipe = publisher | Trigger([&] { counter++; });

        REQUIRE(counter == 0);
        publisher->publish();
        REQUIRE(counter == 1);
    }
}
