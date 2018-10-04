#include "catch2/catch.hpp"

#include "./test_reactive.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("basic relay<T -> T> functions", "[reactive][relay]")
{
    auto publisher = DefaultPublisher();
    auto relay = DefaultRelay();
    auto subscriber = TestSubscriber();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);

    SECTION("on_next")
    {
        publisher->publish(42);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 42);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish(1);
        publisher->error(std::logic_error("a logic error"));
        publisher->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        publisher->publish(1);
        publisher->complete();
        publisher->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}

SCENARIO("basic relay<NoData -> NoData> functions", "[reactive][relay]")
{
    auto publisher = DefaultPublisher<NoData>();
    auto relay = DefaultRelay<NoData>();
    auto subscriber = TestSubscriber<NoData>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);

    SECTION("on_next")
    {
        publisher->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish();
        publisher->error(std::logic_error("a logic error"));
        publisher->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        publisher->publish();
        publisher->complete();
        publisher->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}

SCENARIO("basic relay<T -> NoData> functions", "[reactive][relay]")
{
    auto publisher = DefaultPublisher<int>();
    auto relay = DefaultRelay<int, NoData>();
    auto subscriber = TestSubscriber<NoData>();
    publisher->subscribe(relay);
    relay->subscribe(subscriber);

    SECTION("on_next")
    {
        publisher->publish(7);

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish(7);
        publisher->error(std::logic_error("a logic error"));
        publisher->publish(8);

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        publisher->publish(6);
        publisher->complete();
        publisher->publish(2);

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}
