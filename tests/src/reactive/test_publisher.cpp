#include "catch2/catch.hpp"

#include "test_reactive_utils.hpp"

NOTF_USING_NAMESPACE;

// test cases ======================================================================================================= //

SCENARIO("basic publisher<T> functions", "[reactive][publisher]") {
    SECTION("single subscriber") {
        auto publisher = TestPublisher<int, detail::SinglePublisherPolicy>();
        auto subscriber = TestSubscriber();
        auto subscriber2 = TestSubscriber();

        REQUIRE(publisher->get_subscriber_count() == 0);
        publisher->subscribe(subscriber);

        REQUIRE(!publisher->subscribe(subscriber2));
        REQUIRE(publisher->get_subscriber_count() == 1);

        publisher->publish(1);
        publisher->publish(19);
        REQUIRE(publisher->published.size() == 2);
        REQUIRE(publisher->published[0] == 1);
        REQUIRE(publisher->published[1] == 19);

        {
            auto subscriber3 = TestSubscriber();
            publisher->allow_new_subscribers = false;
            REQUIRE(publisher->get_subscriber_count() == 1);
            publisher->subscribe(subscriber3);
            REQUIRE(publisher->get_subscriber_count() == 1);
            publisher->allow_new_subscribers = true;
        }

        REQUIRE(!publisher->is_completed());
        publisher->complete();
        REQUIRE(publisher->is_completed());
        REQUIRE(publisher->get_subscriber_count() == 0);

        publisher->subscribe(subscriber);
        REQUIRE(publisher->get_subscriber_count() == 0);
    }

    SECTION("multi subscriber") {
        auto publisher = TestPublisher<int, detail::MultiPublisherPolicy>();
        auto subscriber = TestSubscriber();
        publisher->subscribe(subscriber);

        REQUIRE(publisher->get_subscriber_count() == 1);
        {
            auto expiring_subscriber = TestSubscriber();
            publisher->subscribe(expiring_subscriber);
            REQUIRE(publisher->get_subscriber_count() == 2);
        }

        auto subscriber2 = TestSubscriber();
        publisher->subscribe(subscriber2);           // removes expired
        REQUIRE(!publisher->subscribe(subscriber2)); // duplicate
        REQUIRE(publisher->get_subscriber_count() == 2);

        publisher->publish(1);
        publisher->publish(18);
        REQUIRE(publisher->published.size() == 2);
        REQUIRE(publisher->published[0] == 1);
        REQUIRE(publisher->published[1] == 18);
        {
            auto expiring_subscriber = TestSubscriber();
            publisher->subscribe(expiring_subscriber);
            REQUIRE(publisher->get_subscriber_count() == 3);
        }
        publisher->publish(78); // removes expired
        REQUIRE(publisher->get_subscriber_count() == 2);

        {
            auto subscriber3 = TestSubscriber();
            publisher->allow_new_subscribers = false;
            REQUIRE(publisher->get_subscriber_count() == 2);
            publisher->subscribe(subscriber3);
            REQUIRE(publisher->get_subscriber_count() == 2);
            publisher->allow_new_subscribers = true;
        }

        REQUIRE(!publisher->is_completed());
        publisher->complete();
        REQUIRE(publisher->is_completed());
        REQUIRE(publisher->get_subscriber_count() == 0);

        publisher->subscribe(subscriber);
        REQUIRE(publisher->get_subscriber_count() == 0);
    }

    SECTION("single subscriber failure") {
        auto publisher = TestPublisher<int, detail::SinglePublisherPolicy>();
        auto subscriber = TestSubscriber();
        publisher->subscribe(subscriber);

        REQUIRE(!publisher->is_failed());
        publisher->error(std::logic_error(""));
        REQUIRE(publisher->is_failed());
    }

    SECTION("multi subscriber failure") {
        auto publisher = TestPublisher<int, detail::MultiPublisherPolicy>();
        auto subscriber = TestSubscriber();
        publisher->subscribe(subscriber);

        REQUIRE(!publisher->is_failed());
        publisher->error(std::logic_error(""));
        REQUIRE(publisher->is_failed());
    }

    SECTION("fail to subscribe a subscriber of the wrong type") {
        auto publisher = TestPublisher();
        auto subscriber = TestSubscriber<std::string>();
        REQUIRE(!publisher->subscribe(std::static_pointer_cast<AnySubscriber>(subscriber)));
    }

    SECTION("publisher identifier") // for coverage
    {
        REQUIRE(detail::PublisherIdentifier::test<decltype(TestPublisher())>());
        REQUIRE(!detail::PublisherIdentifier::test<decltype(TestSubscriber())>());
    }
}
