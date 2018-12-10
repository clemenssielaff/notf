#include "catch2/catch.hpp"

#include "test/reactive.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("basic subscriber<T> functions", "[reactive][subscriber]") {
    auto publisher = DefaultPublisher();
    auto subscriber = TestSubscriber();
    publisher->subscribe(subscriber);

    SECTION("on_next") {
        publisher->publish(42);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 42);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error") {
        publisher->publish(1);
        publisher->error(std::logic_error("a logic error"));
        publisher->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete") {
        publisher->publish(1);
        publisher->complete();
        publisher->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("publisher identifier") // for coverage
    {
        REQUIRE(!detail::SubscriberIdentifier::test<decltype(DefaultPublisher())>());
        REQUIRE(detail::SubscriberIdentifier::test<decltype(TestSubscriber())>());
    }
}

SCENARIO("default subscriber<T>", "[reactive][subscriber]") {
    auto publisher = DefaultPublisher();
    auto subscriber = DefaultSubscriber();
    publisher->subscribe(subscriber);

    SECTION("on_next") {
        publisher->publish(42); // ignored
    }

    SECTION("on_error") {
        REQUIRE_THROWS_AS(publisher->error(std::logic_error("")), std::exception); // throws
    }

    SECTION("on_complete") {
        publisher->complete(); // ignored
    }
}
