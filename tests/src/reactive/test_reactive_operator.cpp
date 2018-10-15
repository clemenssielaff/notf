#include "catch2/catch.hpp"

#include "./test_reactive.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("basic operator<T -> T> functions", "[reactive][operator]")
{
    auto publisher = DefaultPublisher();
    auto op = DefaultOperator();
    auto subscriber = TestSubscriber();
    publisher->subscribe(op);
    op->subscribe(subscriber);

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

SCENARIO("basic operator<None -> None> functions", "[reactive][operator]")
{
    auto publisher = DefaultPublisher<None>();
    auto op = DefaultOperator<None>();
    auto subscriber = TestSubscriber<None>();
    publisher->subscribe(op);
    op->subscribe(subscriber);

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

SCENARIO("basic operator<T -> None> functions", "[reactive][operator]")
{
    auto publisher = DefaultPublisher<int>();
    auto op = DefaultOperator<int, None>();
    auto subscriber = TestSubscriber<None>();
    publisher->subscribe(op);
    op->subscribe(subscriber);

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

SCENARIO("basic operator<None -> T> functions", "[reactive][operator]")
{
    auto publisher = DefaultGenerator<int>();
    auto subscriber = TestSubscriber<int>();
    publisher->subscribe(subscriber);

    SECTION("publish")
    {
        publisher->publish(485);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 485);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish(45);
        publisher->error(std::logic_error("a logic error"));
        publisher->publish(8);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 45);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        publisher->publish(6);
        publisher->complete();
        publisher->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 6);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_next")
    {
        publisher->publish();
        publisher->publish();
        publisher->publish();
        publisher->publish();

        REQUIRE(subscriber->values.size() == 4);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->values[2] == 3);
        REQUIRE(subscriber->values[3] == 4);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish();
        publisher->publish();
        publisher->on_error(nullptr, std::logic_error(""));
        publisher->publish();

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        publisher->publish();
        publisher->on_complete(nullptr);

        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}
