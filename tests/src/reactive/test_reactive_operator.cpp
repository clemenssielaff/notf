#include "catch2/catch.hpp"

#include "test_reactive.hpp"

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

    SECTION("publish")
    {
        publisher->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("error")
    {
        publisher->publish();
        publisher->error(std::logic_error("a logic error"));
        publisher->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("complete")
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
    auto publisher = DefaultPublisher<None>();
    auto generator = DefaultGenerator<int>();
    auto subscriber = TestSubscriber<int>();
    publisher->subscribe(generator);
    generator->subscribe(subscriber);

    SECTION("publish")
    {
        generator->publish();
        generator->publish();
        generator->publish();
        generator->publish(123);

        REQUIRE(subscriber->values.size() == 4);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->values[2] == 3);
        REQUIRE(subscriber->values[3] == 123);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("error")
    {
        generator->publish(45);
        generator->error(std::logic_error("a logic error"));
        generator->publish(8);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 45);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("complete")
    {
        generator->publish(6);
        generator->complete();
        generator->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 6);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_next")
    {
        publisher->publish();

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        publisher->publish();
        publisher->publish();
        publisher->error(std::logic_error(""));
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
        publisher->publish();
        publisher->complete();
        publisher->publish();

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}

SCENARIO("basic operator<All -> T> functions", "[reactive][operator]")
{
    auto int_publisher = DefaultPublisher<int>();
    auto float_publisher = DefaultPublisher<float>();
    auto op = EverythingRelay<int>();
    auto subscriber = TestSubscriber<int>();
    int_publisher->subscribe(op);
    float_publisher->subscribe(op);
    op->subscribe(subscriber);

    SECTION("publish")
    {
        op->publish();
        op->publish();
        op->publish();
        op->publish(123);

        REQUIRE(subscriber->values.size() == 4);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->values[2] == 3);
        REQUIRE(subscriber->values[3] == 123);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("error")
    {
        op->publish(45);
        op->error(std::logic_error("a logic error"));
        op->publish(8);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 45);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("complete")
    {
        op->publish(6);
        op->complete();
        op->publish(2);

        REQUIRE(subscriber->values.size() == 1);
        REQUIRE(subscriber->values[0] == 6);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_next")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);
        int_publisher->error(std::logic_error(""));
        float_publisher->publish(789.f);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);
        int_publisher->complete();
        float_publisher->publish(789.f);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 2);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}

SCENARIO("basic operator<All -> None> functions", "[reactive][operator]")
{
    auto int_publisher = DefaultPublisher<int>();
    auto float_publisher = DefaultPublisher<float>();
    auto op = EverythingRelay<None>();
    auto subscriber = TestSubscriber<None>();
    int_publisher->subscribe(op);
    float_publisher->subscribe(op);
    op->subscribe(subscriber);

    SECTION("publish")
    {
        op->publish();
        op->publish();
        op->publish();

        REQUIRE(subscriber->counter == 3);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("error")
    {
        op->publish();
        op->error(std::logic_error("a logic error"));
        op->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("complete")
    {
        op->publish();
        op->complete();
        op->publish();

        REQUIRE(subscriber->counter == 1);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_next")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);

        REQUIRE(subscriber->counter == 2);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception == nullptr);
    }

    SECTION("on_error")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);
        int_publisher->error(std::logic_error(""));
        float_publisher->publish(789.f);

        REQUIRE(subscriber->counter == 2);
        REQUIRE(subscriber->is_completed == false);
        REQUIRE(subscriber->exception);
    }

    SECTION("on_complete")
    {
        int_publisher->publish(123);
        float_publisher->publish(456.f);
        int_publisher->complete();
        float_publisher->publish(789.f);

        REQUIRE(subscriber->counter == 2);
        REQUIRE(subscriber->is_completed == true);
        REQUIRE(subscriber->exception == nullptr);
    }
}
