#include "catch.hpp"
#include "test_utils.hpp"

using namespace notf;

TEST_CASE("Working with angles", "[common][float][arithmetic]")
{
    static const float two_pi = pi<float>() * 2;

    SECTION("unormalized angles can can be correctly normalized")
    {
        REQUIRE(norm_angle(-two_pi) == approx(two_pi, precision_low<float>()));
        REQUIRE(norm_angle(-pi<float>()) == approx(pi<float>(), precision_low<float>()));
        REQUIRE(norm_angle(0.) == approx(0.f, precision_low<float>()));
        REQUIRE(norm_angle(pi<float>()) == approx(pi<float>(), precision_low<float>()));
        REQUIRE(norm_angle(two_pi) == approx(0.f, precision_low<float>()));
    }

    SECTION("random angles can be correctly normalized")
    {
        for (auto i = 0; i < 10000; ++i) {
            const float result = norm_angle(random_number<float>());
            REQUIRE(result >= 0);
            REQUIRE(result < two_pi);
        }
    }
}
