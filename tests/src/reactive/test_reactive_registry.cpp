#include "catch2/catch.hpp"

#include "notf/reactive/reactive_registry.hpp"

NOTF_USING_NAMESPACE;

// factories ======================================================================================================== //

auto int_int_relay() { return std::make_shared<Operator<int, int>>(); }
NOTF_REGISTER_REACTIVE_OPERATOR(int_int_relay);

// test cases ======================================================================================================= //

SCENARIO("automatic registration", "[reactive][registry]")
{
    auto any_op = TheReactiveRegistry::create("int_int_relay");
    auto ii_relay = std::dynamic_pointer_cast<Operator<int, int>>(any_op);
    REQUIRE(ii_relay);
}
