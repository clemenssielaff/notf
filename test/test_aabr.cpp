#include "test/catch.hpp"

#include <iostream>

#include "common/aabr.hpp"
#include "common/float.hpp"
#include "common/vector2.hpp"
using namespace notf;

template <typename T>
std::ostream& operator<<(std::ostream& out, const _approx<T>& approxed)
{
    return out << approxed.value;
}

SCENARIO("Aabr can be constructed", "[common][aabr]")
{
    WHEN("you create an Aabr from two vectors")
    {
        const Vector2f top_right   = {1, -1};
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

SCENARIO("Aabr can be modified", "[common][aabr]")
{
    WHEN("you rotate an Aabr")
    {
        const Vector2f bottom_left = {-1, 1};
        const Vector2f top_right   = {1, -1};

        THEN("the resulting aabr will the the aabr of the rotated rect")
        {
            Aabrf aabr_1(top_right, bottom_left);
            Aabrf aabr_2 = aabr_1.get_rotated(static_cast<float>(PI / 4));
            REQUIRE(aabr_2.width() == approx(2 * sqrt(2.f)));
        }
    }
}
