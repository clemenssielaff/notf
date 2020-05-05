#include "catch.hpp"

#include "notf/meta/real.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("functions related to numeric operations", "[meta][numeric]") {
    SECTION("constexpr exponent operator") {
        const double double_v = 3.;
        REQUIRE(is_approx(exp(double_v, 3), 27));
        REQUIRE(is_approx(exp(double_v, 0), 1));

        const size_t size_v = 3;
        REQUIRE(exp(size_v, 3) == 27);
        REQUIRE(exp(size_v, 0) == 1);
    }

    SECTION("clamp") {
        REQUIRE(clamp(-0.1f) == 0.f);
        REQUIRE(clamp(1.1f) == 1.f);
        REQUIRE(clamp(0.1f) == 0.1f);
    }

    SECTION("can be narrow cast") {
        REQUIRE(can_be_narrow_cast<int>(1));
        REQUIRE(can_be_narrow_cast<uint>(1000));
        REQUIRE(!can_be_narrow_cast<uint>(-1));
        REQUIRE(!can_be_narrow_cast<uchar>(1000));
    }
}
