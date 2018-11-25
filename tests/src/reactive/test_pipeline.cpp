#include "catch2/catch.hpp"

#include "notf/reactive/registry.hpp"
#include "test_reactive_utils.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("pipeline", "[reactive][pipeline]") {
    SECTION("l-value publisher => l-value subscriber") {
        auto publisher = DefaultPublisher();
        auto subscriber = TestSubscriber();

        publisher->publish(1);
        {
            auto pipeline = publisher | subscriber;
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 0);
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

    SECTION("l-value publisher => r-value subscriber") {
        auto publisher = DefaultPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | TestSubscriber();
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 0);
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

    SECTION("r-value publisher => l-value subscriber") {
        auto subscriber = TestSubscriber();
        decltype(DefaultPublisher()) publisher;

        {
            auto pipeline = DefaultPublisher() | subscriber;
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 0);
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

    SECTION("r-value publisher => r-value subscriber") {
        decltype(DefaultPublisher()) publisher;
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = DefaultPublisher() | TestSubscriber();
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 0);
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

    SECTION("l-value pipeline => L-value subscriber") {
        auto subscriber = TestSubscriber();
        decltype(DefaultPublisher()) publisher;

        {
            auto pipeline = DefaultPublisher() | DefaultOperator() | DefaultOperator() | subscriber;
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 2);
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

    SECTION("l-value pipeline => r-value subscriber") {
        auto publisher = DefaultPublisher();
        decltype(TestSubscriber()) subscriber;

        {
            auto pipeline = publisher | DefaultOperator() | DefaultOperator() | TestSubscriber();
            REQUIRE(!PipelinePrivate(pipeline).get_first());
            REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 2);
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

    SECTION("pipeline with mixed l- and r-values") {
        SECTION("L->R->L->R->L") {
            auto publisher = DefaultPublisher();
            auto l_value_operator = DefaultOperator();
            auto subscriber = TestSubscriber();
            {
                auto pipeline = publisher | DefaultOperator() | l_value_operator | DefaultOperator() | subscriber;
                REQUIRE(!PipelinePrivate(pipeline).get_first());
                REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 3);

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
        SECTION("R->L->R->L->R") {
            auto first_operator = DefaultOperator();
            auto second_operator = DefaultOperator();
            decltype(DefaultPublisher()) publisher;
            decltype(TestSubscriber()) subscriber;
            {
                auto pipeline = DefaultPublisher() | first_operator | DefaultOperator() | second_operator
                                | TestSubscriber();
                REQUIRE(PipelinePrivate(pipeline).get_functions().size() == 3);
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

    SECTION("pipeline with mixed typed / untyped functions") {
        SECTION("L -> UL -> L") {
            auto i_publisher = TestPublisher<int>();
            auto i_subscriber = TestSubscriber<int>();
            auto ii_relay = TheReactiveRegistry::create("IIRelay");

            auto pipeline = i_publisher | ii_relay | i_subscriber;
            REQUIRE(i_subscriber->values.empty());
            i_publisher->publish(234);
            REQUIRE(i_subscriber->values.size() == 1);
            REQUIRE(i_subscriber->values[0] == 234);
        }
        SECTION("L -> UR -> L") {
            auto i_publisher = TestPublisher<int>();
            auto i_subscriber = TestSubscriber<int>();

            auto pipeline = i_publisher | TheReactiveRegistry::create("IIRelay") | i_subscriber;
            REQUIRE(i_subscriber->values.empty());
            i_publisher->publish(234);
            REQUIRE(i_subscriber->values.size() == 1);
            REQUIRE(i_subscriber->values[0] == 234);
        }
        SECTION("R -> UR -> UR -> L") {
            decltype(TestPublisher<int>()) i_publisher;
            auto i_subscriber = TestSubscriber<int>();

            auto pipeline = TestPublisher<int>() | TheReactiveRegistry::create("IIRelay")
                            | TheReactiveRegistry::create("IIRelay") | i_subscriber;
            i_publisher
                = std::dynamic_pointer_cast<decltype(i_publisher)::element_type>(PipelinePrivate(pipeline).get_first());
            REQUIRE(i_publisher);
            REQUIRE(i_subscriber->values.empty());
            i_publisher->publish(234);
            REQUIRE(i_subscriber->values.size() == 1);
            REQUIRE(i_subscriber->values[0] == 234);
        }
        SECTION("L -> UR -> R -> UL -> R") {
            auto i_publisher = TestPublisher<int>();
            auto ii_relay = TheReactiveRegistry::create("IIRelay");
            decltype(TestSubscriber<int>()) i_subscriber;

            auto pipeline = i_publisher | TheReactiveRegistry::create("IIRelay") | DefaultOperator() | ii_relay
                            | TestSubscriber();
            i_subscriber
                = std::dynamic_pointer_cast<decltype(i_subscriber)::element_type>(PipelinePrivate(pipeline).get_last());
            REQUIRE(i_subscriber);
            REQUIRE(i_subscriber->values.empty());
            i_publisher->publish(234);
            REQUIRE(i_subscriber->values.size() == 1);
            REQUIRE(i_subscriber->values[0] == 234);
        }
        SECTION("failure if you try to connect a wrong type to an untyped typeline") {
            REQUIRE_THROWS_AS(TestPublisher<int>() | TheReactiveRegistry::create("IIRelay")
                                  | DefaultOperator<std::string>(),
                              PipelineError);
        }
        SECTION("failure if the pipeline closes with a subscriber") {
            // we really have to try to create an untyped subscriber that is not also an operator,
            // I don't expect this to happen in production but you never know...
            REQUIRE_THROWS_AS(TestPublisher<int>()
                                  | std::dynamic_pointer_cast<AnySubscriber>(TestSubscriber<std::string>()),
                              PipelineError);
            REQUIRE_THROWS_AS(TestPublisher<int>() | std::dynamic_pointer_cast<AnyOperator>(TestSubscriber<int>()),
                              PipelineError);
        }
        SECTION("failure if you try to attach a nullptr") {
            REQUIRE_THROWS_AS(TestPublisher<int>() | AnyOperatorPtr{}, PipelineError);
            REQUIRE_THROWS_AS(TestPublisher<int>() | std::make_shared<AnyOperator>(), PipelineError);
            REQUIRE_THROWS_AS(TestPublisher() | DefaultOperator() | AnyOperatorPtr{}, PipelineError);
            REQUIRE_THROWS_AS(TestPublisher() | DefaultOperator() | std::make_shared<AnyOperator>(), PipelineError);
        }
    }
}
