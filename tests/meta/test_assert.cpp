#include "catch.hpp"

#include "notf/meta/assert.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("assert", "[meta][assert]") {
    SECTION("basic (throwing) assertion macro") {
#ifdef NOTF_DEBUG
        REQUIRE_THROWS_AS(NOTF_ASSERT(false), AssertionError);
        REQUIRE_THROWS_AS(NOTF_ASSERT(false, "with message"), AssertionError);
#endif
    }
}
