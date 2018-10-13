#include "catch2/catch.hpp"

#include "notf/reactive/pipeline.hpp"
#include "notf/reactive/reactive_registry.hpp"

#include "./test_reactive.hpp"

NOTF_USING_NAMESPACE;

struct TestOperatorFactory : public AnyOperatorFactory {
    AnyOperatorPtr _create(std::vector<std::any>&&) const final { return {}; }
};

// factories ======================================================================================================== //

auto int_int_relay() { return std::make_shared<Operator<int, int>>(); }
NOTF_REGISTER_REACTIVE_OPERATOR(int_int_relay);

// test cases ======================================================================================================= //

SCENARIO("automatic registration", "[reactive][registry]")
{
    SECTION("check name")
    {
        REQUIRE(TheReactiveRegistry::has_operator("int_int_relay"));
        REQUIRE(!TheReactiveRegistry::has_operator("definetly not an operator, I hope"));

        TestOperatorFactory test_factory; // to cover the AnyOperatorFactory::~AnyOperatorFactory() line...
    }

    SECTION("untyped factory")
    {
        auto any_op = TheReactiveRegistry::create("int_int_relay");
        auto ii_relay = std::dynamic_pointer_cast<Operator<int, int>>(any_op);
        REQUIRE(ii_relay);

        REQUIRE_THROWS_AS(TheReactiveRegistry::create("definetly not an operator, I hope"), notf::out_of_bounds);
        REQUIRE_THROWS_AS(TheReactiveRegistry::create("int_int_relay", 123.4), notf::value_error);
    }

    SECTION("casting factory")
    {
        REQUIRE(TheReactiveRegistry::create<int>("int_int_relay"));
        REQUIRE(!TheReactiveRegistry::create<float>("int_int_relay"));
        REQUIRE(!TheReactiveRegistry::create<int, float>("int_int_relay"));
        REQUIRE(!TheReactiveRegistry::create<int, int, detail::MultiPublisherPolicy>("int_int_relay"));
        REQUIRE(!TheReactiveRegistry::create<int>("definetly not an operator, I hope"));
        REQUIRE_THROWS_AS(TheReactiveRegistry::create<int>("int_int_relay", 123.4), notf::value_error);
    }
}

SCENARIO("working with untyped operators", "[reactive][registry]")
{
    SECTION("work as expected if produced with the casting factory")
    {
        auto i_publisher = TestPublisher<int>();
        auto i_subscriber = TestSubscriber<int>();
        auto ii_relay = TheReactiveRegistry::create<int>("int_int_relay");

        auto pipeline = i_publisher | ii_relay | i_subscriber;
        REQUIRE(i_subscriber->values.empty());
        i_publisher->publish(234);
        REQUIRE(i_subscriber->values.size() == 1);
        REQUIRE(i_subscriber->values[0] == 234);
    }
}
