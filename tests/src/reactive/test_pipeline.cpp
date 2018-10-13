#include "catch2/catch.hpp"

#include "./test_reactive.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("pipeline", "[reactive][pipeline]")
{
    SECTION("l-value publisher => l-value subscriber")
    {
        auto publisher = DefaultPublisher();
        auto subscriber = TestSubscriber();

        publisher->publish(1);
        {
            auto pipeline = publisher | subscriber;
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 0);
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
        auto publisher = DefaultPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | TestSubscriber();
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 0);
            subscriber = PipelinePrivate(pipeline).get_last();

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
        decltype(DefaultPublisher()) publisher;

        {
            auto pipeline = DefaultPublisher() | subscriber;
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 0);
            publisher
                = std::dynamic_pointer_cast<decltype(publisher)::element_type>(PipelinePrivate(pipeline).get_first());
            REQUIRE(publisher);

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
        decltype(DefaultPublisher()) publisher;
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = DefaultPublisher() | TestSubscriber();
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 0);
            subscriber = PipelinePrivate(pipeline).get_last();
            publisher
                = std::dynamic_pointer_cast<decltype(publisher)::element_type>(PipelinePrivate(pipeline).get_first());
            REQUIRE(publisher);

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
        decltype(DefaultPublisher()) publisher;

        {
            auto pipeline = DefaultPublisher() | DefaultOperator() | DefaultOperator() | subscriber;
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 2);
            publisher
                = std::dynamic_pointer_cast<decltype(publisher)::element_type>(PipelinePrivate(pipeline).get_first());
            REQUIRE(publisher);

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
        auto publisher = DefaultPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | DefaultOperator() | DefaultOperator() | TestSubscriber();
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 2);
            subscriber = PipelinePrivate(pipeline).get_last();

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

    SECTION("pipeline with mixed l- and r-values")
    {
        SECTION("L->R->L->R->L")
        {
            auto publisher = DefaultPublisher();
            auto l_value_operator = DefaultOperator();
            auto subscriber = TestSubscriber();
            {
                auto pipeline = publisher | DefaultOperator() | l_value_operator | DefaultOperator() | subscriber;
                REQUIRE(!PipelinePrivate(pipeline).get_first());
                REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 3);

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
        SECTION("R->L->R->L->R")
        {
            auto first_operator = DefaultOperator();
            auto second_operator = DefaultOperator();
            decltype(DefaultPublisher()) publisher;
            decltype(TestSubscriber()) subscriber;
            {
                auto pipeline = DefaultPublisher() | first_operator | DefaultOperator() | second_operator
                                | TestSubscriber();
                REQUIRE(PipelinePrivate(pipeline).get_elements().size() == 3);
                publisher = std::dynamic_pointer_cast<decltype(publisher)::element_type>(
                    PipelinePrivate(pipeline).get_first());
                REQUIRE(publisher);
                subscriber = PipelinePrivate(pipeline).get_last();

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
    }
}
