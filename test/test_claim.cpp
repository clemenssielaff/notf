#include "catch.hpp"

#include "app/widget/claim.hpp"
#include "test_utils.hpp"
using namespace notf;

SCENARIO("Claim", "[core][claim]")
{
    Claim claim;

    WHEN("you set a fixed stretch claim")
    {
        const float width = random_number<float>(0, highest_tested<float>());
        const float height = random_number<float>(0, highest_tested<float>());

        claim.get_horizontal().set_fixed(width);
        claim.get_vertical().set_fixed(height);

        THEN("all 3 values will be the same")
        {
            REQUIRE(is_approx(claim.get_horizontal().get_min(), width));
            REQUIRE(is_approx(claim.get_horizontal().get_preferred(), width));
            REQUIRE(is_approx(claim.get_horizontal().get_max(), width));

            REQUIRE(is_approx(claim.get_vertical().get_min(), height));
            REQUIRE(is_approx(claim.get_vertical().get_preferred(), height));
            REQUIRE(is_approx(claim.get_vertical().get_max(), height));
        }
    }
}
