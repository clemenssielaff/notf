#include "test/catch.hpp"

#include "common/aabr.hpp"
#include "common/vector2.hpp"
using notf::Aabr;
using notf::Vector2;

SCENARIO("Aabr can be constructed", "[common][aabr]")
{
    WHEN("you create an Aabr from two vectors")
    {
        const Vector2 top_right = {1, -1};
        const Vector2 bottom_left = {-1, 1};

        THEN("the constructor will place the corners correctly")
        {
            Aabr aabr_1 (top_right, bottom_left);
            Aabr aabr_2 (bottom_left, top_right);
            REQUIRE(aabr_1.left() == -1.f);
            REQUIRE(aabr_1.right() == 1.f);
            REQUIRE(aabr_1.top() == -1.f);
            REQUIRE(aabr_1.bottom() == 1.f);
            REQUIRE(aabr_1 == aabr_2);
        }
    }
}
