#include "catch2/catch.hpp"

#include "./test_reactive.hpp"

NOTF_USING_NAMESPACE;

template<class Last>
struct notf::Accessor<Pipeline<Last>, Tester> {

    Accessor(Pipeline<Last>& pipeline) : m_pipeline(pipeline) {}

    using Operators = typename Pipeline<Last>::Operators;
    const Operators& get_operators() { return m_pipeline.m_operators; }
    Last& get_last_operator() { return m_pipeline.m_last; }

    Pipeline<Last>& m_pipeline;
};
template<class Pipe, class P = std::decay_t<Pipe>>
auto PipelinePrivate(Pipe&& pipeline)
{
    return notf::Accessor<Pipeline<typename P::last_t>, Tester>{std::forward<Pipe>(pipeline)};
}

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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 0);
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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 0);
            subscriber = PipelinePrivate(pipeline).get_last_operator();

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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 1);
            publisher = std::dynamic_pointer_cast<decltype(publisher)::element_type>(
                PipelinePrivate(pipeline).get_operators()[0]);

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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 1);
            subscriber = PipelinePrivate(pipeline).get_last_operator();
            publisher = std::dynamic_pointer_cast<decltype(publisher)::element_type>(
                PipelinePrivate(pipeline).get_operators()[0]);

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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 3);
            publisher = std::dynamic_pointer_cast<decltype(publisher)::element_type>(
                PipelinePrivate(pipeline).get_operators()[0]);

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
            REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 2);
            subscriber = PipelinePrivate(pipeline).get_last_operator();

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
                REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 3);

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
                REQUIRE(PipelinePrivate(pipeline).get_operators().size() == 4);

                publisher = std::dynamic_pointer_cast<decltype(publisher)::element_type>(
                    PipelinePrivate(pipeline).get_operators()[0]);
                subscriber = PipelinePrivate(pipeline).get_last_operator();

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
