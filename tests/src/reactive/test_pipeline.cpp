#include "catch2/catch.hpp"

#include "notf/reactive/pipeline.hpp"

NOTF_USING_NAMESPACE;

namespace {

auto TestPublisher() { return std::make_shared<Publisher<int, detail::SinglePublisherPolicy>>(); }

auto TestSubscriber()
{
    struct TestSubscriberImpl : public Subscriber<int> {

        void on_next(const detail::PublisherBase*, const int& value) final { values.emplace_back(value); }

        void on_error(const std::exception& error) final
        {
            try {
                throw error;
            }
            catch (...) {
                exception = std::current_exception();
            };
        }

        void on_complete() final { is_completed = true; }

        std::vector<int> values;
        std::exception_ptr exception;
        bool is_completed = false;
    };
    return std::make_shared<TestSubscriberImpl>();
}

auto TestRelay() { return std::make_shared<Relay<int, detail::SinglePublisherPolicy>>(); }

} // namespace

// test cases ======================================================================================================= //

SCENARIO("pipeline", "[reactive][pipeline]")
{
    SECTION("l-value publisher => l-value subscriber")
    {
        auto publisher = TestPublisher();
        auto subscriber = TestSubscriber();

        publisher->publish(1);
        {
            auto pipeline = publisher | subscriber;
            REQUIRE(pipeline.get_operator_count() == 1);
            publisher->publish(2);

            pipeline.disable();
            publisher->publish(3);
            pipeline.enable();

            publisher->publish(4);
        }
        publisher->publish(5);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 2);
        REQUIRE(subscriber->values[1] == 4);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed == false);
    }

    SECTION("l-value publisher => r-value subscriber")
    {
        auto publisher = TestPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | TestSubscriber();
            REQUIRE(pipeline.get_operator_count() == 2);
            subscriber = pipeline.get_last_operator();

            publisher->publish(1);

            pipeline.disable();
            publisher->publish(2);
            pipeline.enable();

            publisher->publish(3);
        }
        publisher->publish(4);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 3);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed == false);
    }

    SECTION("r-value publisher => l-value subscriber")
    {
        auto subscriber = TestSubscriber();
        decltype(TestPublisher()) publisher;

        {
            auto pipeline = TestPublisher() | subscriber;
            REQUIRE(pipeline.get_operator_count() == 2);
            publisher = pipeline.get_first_operator();

            publisher->publish(1);

            pipeline.disable();
            publisher->publish(2);
            pipeline.enable();

            publisher->publish(3);
        }
        publisher->publish(4);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 3);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed == false);
    }

    SECTION("r-value publisher => r-value subscriber")
    {
        decltype(TestSubscriber()) subscriber;
        decltype(TestPublisher()) publisher;

        {
            auto pipeline = TestPublisher() | TestSubscriber();
            REQUIRE(pipeline.get_operator_count() == 3);
            subscriber = pipeline.get_last_operator();
            publisher = pipeline.get_first_operator();

            publisher->publish(1);

            pipeline.disable();
            publisher->publish(2);
            pipeline.enable();

            publisher->publish(3);
        }
        publisher->publish(4);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 3);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed == false);
    }

    SECTION("l-value pipeline => L-value subscriber")
    {
        auto subscriber = TestSubscriber();
        decltype(TestPublisher()) publisher;

        {
            auto pipeline = TestPublisher() | TestRelay() | TestRelay() | subscriber;
            REQUIRE(pipeline.get_operator_count() == 4);
            publisher = pipeline.get_first_operator();

            publisher->publish(1);

            pipeline.disable();
            publisher->publish(2);
            pipeline.enable();

            publisher->publish(3);
        }
        publisher->publish(4);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 3);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed == false);
    }

    SECTION("l-value pipeline => r-value subscriber")
    {
        auto publisher = TestPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | TestRelay() | TestRelay() | TestSubscriber();
            REQUIRE(pipeline.get_operator_count() == 4);
            subscriber = pipeline.get_last_operator();

            publisher->publish(1);

            pipeline.disable();
            publisher->publish(2);
            pipeline.enable();

            publisher->publish(3);
        }
        publisher->publish(4);

        REQUIRE(subscriber->values.size() == 2);
        REQUIRE(subscriber->values[0] == 1);
        REQUIRE(subscriber->values[1] == 3);
        REQUIRE(subscriber->exception == nullptr);
        REQUIRE(subscriber->is_completed ==  false);
    }

    SECTION("additional toggle operators count against the operator count")
    //... not that you would ever do that on purpose... but just in case
    {
        auto pipeline = std::make_shared<detail::TogglePipelineOperator<int>>()
                        | std::make_shared<detail::TogglePipelineOperator<int>>();
        REQUIRE(pipeline.get_operator_count() == 3);
    }
}
