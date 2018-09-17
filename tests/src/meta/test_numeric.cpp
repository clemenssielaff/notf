#include "catch2/catch.hpp"

#include "notf/meta/real.hpp"

NOTF_USING_META_NAMESPACE;

SCENARIO("functions related to numeric operations", "[meta][numeric]")
{
    SECTION("constexpr exponent operator")
    {
        const double double_v = 3.;
        REQUIRE(is_approx(exp(double_v, 3), 27));
        REQUIRE(is_approx(exp(double_v, 0), 1));

        const size_t size_v = 3;
        REQUIRE(exp(size_v, 3) ==  27);
        REQUIRE(exp(size_v, 0) == 1);
    }

    SECTION("fun with digits")
    {
        REQUIRE(get_digit(123, 0) == 3);
        REQUIRE(get_digit(123, 1) == 2);
        REQUIRE(get_digit(123, 2) == 1);
        REQUIRE(get_digit(123, 3) == 0);


        REQUIRE(count_digits(8) == 1);
        REQUIRE(count_digits(123) == 3);
        REQUIRE(count_digits(1234) == 4);
    }
}
