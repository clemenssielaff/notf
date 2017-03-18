#include "test/catch.hpp"

#include "common/float.hpp"
#include "test/test_utils.hpp"
using namespace notf;

SCENARIO("Working with angles", "[common][float][arithmetic]")
{
    WHEN("an unormalized real value is given")
    {
        static const float pi     = static_cast<float>(PI);
        static const float two_pi = static_cast<float>(TWO_PI);

        THEN("it can can be correctly normalized")
        {
            REQUIRE(norm_angle(-two_pi) == approx(0.f, precision_low<float>()));
            REQUIRE(norm_angle(-pi) == approx(pi, precision_low<float>()));
            REQUIRE(norm_angle(0.) == approx(0.f, precision_low<float>()));
            REQUIRE(norm_angle(pi) == approx(-pi, precision_low<float>()));
            REQUIRE(norm_angle(two_pi) == approx(0.f, precision_low<float>()));
        }

        THEN("random numbers can also be correctly normalized")
        {
            for (auto i = 0; i < 10000; ++i) {
                const float result = norm_angle(random_number<float>());
                REQUIRE(-pi <= result);
                REQUIRE(result <= pi);
            }
        }
    }
}
