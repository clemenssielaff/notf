#include "catch2/catch.hpp"

#include "notf/reactive/pipeline.hpp"
#include "notf/reactive/registry.hpp"

#include "test_reactive_utils.hpp"

NOTF_USING_NAMESPACE;

struct TestOperatorFactory : public AnyOperatorFactory {
    AnyOperatorPtr _create(std::vector<std::any>&&) const final { return {}; }
};

// factories ======================================================================================================== //

auto IIRelay() { return std::make_shared<Operator<int, int>>(); }
NOTF_REGISTER_REACTIVE_OPERATOR(IIRelay);

auto StepCounter(size_t start) {
    struct StepCounterImpl : public Operator<None, int> {
        NOTF_UNUSED StepCounterImpl(size_t start) : m_counter(start) {}
        void on_next(const AnyPublisher* /*publisher*/) override { this->publish(static_cast<int>(m_counter++ * 2)); }
        size_t m_counter;
    };
    return std::make_shared<StepCounterImpl>(start);
}
NOTF_REGISTER_REACTIVE_OPERATOR(StepCounter);

// test cases ======================================================================================================= //

SCENARIO("automatic registration", "[reactive][registry]") {
    SECTION("check name") {
        REQUIRE(TheReactiveRegistry::has_operator("IIRelay"));
        REQUIRE(!TheReactiveRegistry::has_operator("definetly not an operator, I hope"));

        TestOperatorFactory test_factory; // to cover the AnyOperatorFactory::~AnyOperatorFactory() line...
    }

    SECTION("untyped factory") {
        auto any_op = TheReactiveRegistry::create("IIRelay");
        auto ii_relay = std::dynamic_pointer_cast<Operator<int, int>>(any_op);
        REQUIRE(ii_relay);

        REQUIRE_THROWS_AS(TheReactiveRegistry::create("definetly not an operator, I hope"), notf::OutOfBounds);
        REQUIRE_THROWS_AS(TheReactiveRegistry::create("IIRelay", 123.4), notf::ValueError);

        REQUIRE_THROWS_AS(TheReactiveRegistry::create("StepCounter"), notf::ValueError);
        REQUIRE_THROWS_AS(TheReactiveRegistry::create("StepCounter", All{}), notf::ValueError);
    }

    SECTION("casting factory") {
        REQUIRE(TheReactiveRegistry::create<int>("IIRelay"));
        REQUIRE(!TheReactiveRegistry::create<float>("IIRelay"));
        REQUIRE(!TheReactiveRegistry::create<int, float>("IIRelay"));
        REQUIRE(!TheReactiveRegistry::create<int, int, detail::MultiPublisherPolicy>("IIRelay"));
        REQUIRE(!TheReactiveRegistry::create<int>("definetly not an operator, I hope"));
        REQUIRE_THROWS_AS(TheReactiveRegistry::create<int>("IIRelay", 123.4), notf::ValueError);

        REQUIRE(TheReactiveRegistry::create<None, int>("StepCounter", 48));
        REQUIRE(!TheReactiveRegistry::create<None, float>("StepCounter", 48));
        REQUIRE_THROWS_AS((TheReactiveRegistry::create<None, int>("StepCounter")), notf::ValueError);
        REQUIRE_THROWS_AS((TheReactiveRegistry::create<None, int>("StepCounter", All{})), notf::ValueError);
    }
}

SCENARIO("working with untyped operators", "[reactive][registry]") {
    SECTION("work as expected if produced with the casting factory") {
        auto i_publisher = TestPublisher<int>();
        auto i_subscriber = TestSubscriber<int>();
        auto ii_relay = TheReactiveRegistry::create<int>("IIRelay");

        auto pipeline = i_publisher | ii_relay | i_subscriber;
        REQUIRE(i_subscriber->values.empty());
        i_publisher->publish(234);
        REQUIRE(i_subscriber->values.size() == 1);
        REQUIRE(i_subscriber->values[0] == 234);
    }
}
