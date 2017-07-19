#include "catch.hpp"

#include "core/claim.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("Claim", "[core][claim]")
{
    Claim claim;

    WHEN("you set a fixed stretch claim")
    {
        const float width = random_number<float>();
        const float height = random_number<float>();

        claim.get_horizontal().set_fixed(width);
        claim.get_vertical().set_fixed(height);

        THEN("all 3 values will be the same")
        {
            REQUIRE(claim.get_horizontal().get_min() == width);
            REQUIRE(claim.get_horizontal().get_preferred() == width);
            REQUIRE(claim.get_horizontal().get_max() == width);

            REQUIRE(claim.get_vertical().get_min() == height);
            REQUIRE(claim.get_vertical().get_preferred() == height);
            REQUIRE(claim.get_vertical().get_max() == height);
        }
    }
}
