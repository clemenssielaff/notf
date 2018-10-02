#include "catch2/catch.hpp"

#include "notf/meta/assert.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("assert", "[meta][assert]")
{
    SECTION("basic (throwing) assertion macro")
    {
        REQUIRE_THROWS_AS(NOTF_ASSERT(false), assertion_error);
        REQUIRE_THROWS_AS(NOTF_ASSERT(false, "with message"), assertion_error);
    }
}
