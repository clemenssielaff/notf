#include "test/catch.hpp"

#include "common/aabr.hpp"
#include "common/float.hpp"
#include "common/vector2.hpp"
using namespace notf;

SCENARIO("Aabr can be constructed", "[common][aabr]")
{
    WHEN("you create an Aabr from two vectors")
    {
        const Vector2f top_right = {1, -1};
        const Vector2f bottom_left = {-1, 1};

        THEN("the constructor will place the corners correctly")
        {
            Aabrf aabr_1(top_right, bottom_left);
            Aabrf aabr_2(bottom_left, top_right);
            REQUIRE(aabr_1.left() == approx(-1));
            REQUIRE(aabr_1.right() == 1.f);
            REQUIRE(aabr_1.top() == approx(-1));
            REQUIRE(aabr_1.bottom() == 1.f);
            REQUIRE(aabr_1 == aabr_2);
        }
    }
}
