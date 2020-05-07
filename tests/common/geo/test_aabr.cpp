#include "catch.hpp"

#include "notf/common/geo/aabr.hpp"
#include "notf/common/geo/matrix3.hpp"

using namespace notf;

TEST_CASE("Construct Aabr", "[common][aabr]") {
    SECTION("from two vectors") {
        const V2f top_right = {1, 1};
        const V2f bottom_left = {-1, -1};

        Aabrf aabr_1(top_right, bottom_left);
        Aabrf aabr_2(bottom_left, top_right);
        REQUIRE(is_approx(aabr_1.get_left(), -1));
        REQUIRE(is_approx(aabr_1.get_right(), 1.f));
        REQUIRE(is_approx(aabr_1.get_top(), 1.f));
        REQUIRE(is_approx(aabr_1.get_bottom(), -1.f));
        REQUIRE(aabr_1 == aabr_2);
    }
}

TEST_CASE("Modify Aabrs", "[common][aabr]") {
    SECTION("rotation") {
        const V2f bottom_left = {-1, -1};
        const V2f top_right = {1, 1};

        Aabrf aabr_1(top_right, bottom_left);
        M3f rotation = M3f::rotation(pi<float>() / 4);
        Aabrf aabr_2 = transform_by(aabr_1, rotation);
        REQUIRE(is_approx(aabr_2.get_width(), 2 * sqrt(2.f)));
    }
}
