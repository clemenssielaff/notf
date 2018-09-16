#include "catch2/catch.hpp"

#include "notf/meta/real.hpp"

NOTF_USING_META_NAMESPACE;

SCENARIO("functions related to floating-point calculation", "[meta][real]")
{
    SECTION("check constants")
    {
        REQUIRE(is_approx(pi<double>(), 3.141592653589793238462643383279502884197169399375105820975));
        REQUIRE(is_approx(kappa<double>(), 0.552284749830793398402251632279597438092895833835930764235));
        REQUIRE(is_approx(phi<double>(), 1.618033988749894848204586834365638117720309179805762862135));
    }

    SECTION("check real validity")
    {
        REQUIRE(is_inf(INFINITY));
        REQUIRE(!is_inf(pi()));
        REQUIRE(!is_inf(123));

        REQUIRE(is_nan(NAN));
        REQUIRE(!is_nan(phi()));
        REQUIRE(!is_nan(123));

        REQUIRE(is_real(123.));
        REQUIRE(!is_real(INFINITY));
        REQUIRE(!is_real(NAN));
    }

    SECTION("check real")
    {
        REQUIRE(is_zero(0.));
        REQUIRE(!is_zero(1.));

        REQUIRE(sign(-1.) < 0);
        REQUIRE(sign(153.) > 0);
        REQUIRE(sign(0.) > 0);
        REQUIRE(sign(-0.) < 0);
    }

    SECTION("is approx can be used to fuzzy compare real numbers")
    {
        REQUIRE(!is_approx(0.1, NAN));
        REQUIRE(!is_approx(0.1, INFINITY));
        REQUIRE(is_approx(0.1, 0.10000000000000001));
        REQUIRE(is_approx(183716818.8719874, 183716818.8719875));
        REQUIRE(!is_approx(NAN, NAN));
        REQUIRE(is_approx(INFINITY, INFINITY));

        REQUIRE(precision_low<short>() == 0);
        REQUIRE(precision_low<int>() == 0);
        REQUIRE(precision_low<int>() == 0);
        REQUIRE(precision_high<short>() == 0);
        REQUIRE(precision_high<int>() == 0);
    }
}
