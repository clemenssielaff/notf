#include "catch2/catch.hpp"

#include "notf/meta/real.hpp"

#include "test_utils.hpp"

namespace {
const float two_pi = notf::pi<float>() * 2;
}

NOTF_USING_NAMESPACE;

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
        constexpr double double_nan = static_cast<double>(NAN);
        constexpr double double_inf = static_cast<double>(INFINITY);

        REQUIRE(!is_approx(0.1f, NAN));
        REQUIRE(!is_approx(NAN, 1.68f));
        REQUIRE(!is_approx(NAN, NAN));
        REQUIRE(is_approx(INFINITY, INFINITY));
        REQUIRE(!is_approx(INFINITY, 85.568f));
        REQUIRE(!is_approx(0.578f, INFINITY));
        REQUIRE(is_approx(0.1f, 0.1000001f));
        REQUIRE(is_approx(9999831998412.2f, 9999831998412.1f));
        REQUIRE(!is_approx(838412.f, 838413.f));
        REQUIRE(!is_approx(838413.f, 838412.f));
        REQUIRE(is_approx(9998413.f, 9998412.f));

        REQUIRE(!is_approx(0.1, double_nan));
        REQUIRE(!is_approx(double_nan, 1.68));
        REQUIRE(!is_approx(double_inf, 85.568));
        REQUIRE(!is_approx(0.578, double_inf));
        REQUIRE(is_approx(double_inf, double_inf));
        REQUIRE(is_approx(0.1, 0.10000000000000001));
        REQUIRE(is_approx(183716818.8719874, 183716818.8719875));
        REQUIRE(is_approx(183716818.8719875, 183716818.8719874));
        REQUIRE(!is_approx(183716818.8719876, 183716818.8719874));

        REQUIRE(!is_approx(double_nan, 1));
        REQUIRE(!is_approx(double_inf, 85));
        REQUIRE(!is_approx(double_inf, 0));
        REQUIRE(is_approx(0.00000000000000001, 0));
        REQUIRE(is_approx(183716818.9999999, 183716819));
        REQUIRE(is_approx(183716818.0000001, 183716818));
        REQUIRE(!is_approx(183716818.000001, 183716818));

        REQUIRE(!is_approx(1, double_nan));
        REQUIRE(!is_approx(85, double_inf));
        REQUIRE(!is_approx(0, double_inf));
        REQUIRE(is_approx(0, 0.00000000000000001));
        REQUIRE(is_approx(183716819, 183716818.9999999));
        REQUIRE(is_approx(183716818, 183716818.0000001));
        REQUIRE(!is_approx(183716818, 183716818.000001));

        REQUIRE(!is_approx(double_nan, 1));
        REQUIRE(!is_approx(double_inf, 85));
        REQUIRE(!is_approx(double_inf, 0));
        REQUIRE(is_approx(0.00000000000000001, 0));
        REQUIRE(is_approx(183716818.9999999, 183716819));
        REQUIRE(is_approx(183716818.0000001, 183716818));
        REQUIRE(!is_approx(183716818.000001, 183716818));

        REQUIRE(precision_low<short>() == 0);
        REQUIRE(precision_low<int>() == 0);
        REQUIRE(precision_low<int>() == 0);
        REQUIRE(precision_high<short>() == 0);
        REQUIRE(precision_high<int>() == 0);
    }

    SECTION("unormalized angles can can be correctly normalized")
    {
        REQUIRE(is_approx(norm_angle(-two_pi), norm_angle(two_pi), precision_low<float>()));
        REQUIRE(is_approx(norm_angle(-pi<float>()), pi<float>(), precision_low<float>()));
        REQUIRE(is_approx(norm_angle(0.), 0.f, precision_low<float>()));
        REQUIRE(is_approx(norm_angle(pi<float>()), pi<float>(), precision_low<float>()));
        REQUIRE(is_approx(norm_angle(two_pi), 0.f, precision_low<float>()));
    }

    SECTION("random angles can be correctly normalized")
    {
        for (auto i = 0; i < 10000; ++i) {
            const float result = norm_angle(random_tested<float>());
            REQUIRE(result >= 0);
            REQUIRE(result < two_pi);
        }
    }

    SECTION("the degree literal converts degrees into radians")
    {
        REQUIRE(is_approx(0_deg, 0, precision_low<float>()));
        REQUIRE(is_approx(90._deg, pi<long double>() * 0.5l, precision_high<long double>()));
        REQUIRE(is_approx(180_deg, pi<long double>(), precision_high<long double>()));
        REQUIRE(is_approx(270_deg, pi<long double>() * 1.5l, precision_high<long double>()));
    }
}
