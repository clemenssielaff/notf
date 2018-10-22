#include "catch2/catch.hpp"

#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("types", "[meta][types]")
{
    SECTION("None")
    {
        REQUIRE(None() == None());
        REQUIRE(!(None() < None()));
    }

    SECTION("All")
    {
        REQUIRE(All() == All());
        REQUIRE(!(All() < All()));
    }

    SECTION("all")
    {
        REQUIRE(all(1, true, 1 == 1));
        REQUIRE(!all(1, false, 1 == 1));
    }

    SECTION("any")
    {
        REQUIRE(any(0, true, 1 == 2));
        REQUIRE(!any(0, false, 1 == 2));
    }
}
