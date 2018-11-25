#include "catch2/catch.hpp"

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
}
