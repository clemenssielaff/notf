#include "catch.hpp"

#include "common/aabr.hpp"
#include "common/float.hpp"
#include "common/vector2.hpp"
using namespace notf;

TEST_CASE("Construct Aabr", "[common][aabr]")
{
    SECTION("from two vectors")
    {
        const Vector2f top_right   = {1, 1};
        const Vector2f bottom_left = {-1, -1};

        Aabrf aabr_1(top_right, bottom_left);
        Aabrf aabr_2(bottom_left, top_right);
        REQUIRE(aabr_1.left() == approx(-1));
        REQUIRE(aabr_1.right() == 1.f);
        REQUIRE(aabr_1.top() == approx(1));
        REQUIRE(aabr_1.bottom() == approx(-1.f));
        REQUIRE(aabr_1 == aabr_2);
    }
}

TEST_CASE("Modify Aabrs", "[common][aabr]")
{
    SECTION("rotation")
    {
        const Vector2f bottom_left = {-1, -1};
        const Vector2f top_right   = {1, 1};

        Aabrf aabr_1(top_right, bottom_left);
        Matrix3f rotation = Matrix3f::rotation(pi<float>() / 4);
        Aabrf aabr_2      = rotation.transform(aabr_1);
        REQUIRE(aabr_2.width() == approx(2 * sqrt(2.f)));
    }
}
