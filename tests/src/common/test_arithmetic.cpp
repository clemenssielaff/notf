#include "catch2/catch.hpp"

#include "notf/common/arithmetic.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Arithmetic types", "[common][arithmetic]")
{
//    SECTION("TestVector4)")
//    {
//        SECTION("Construction")
//        {
//            NOTF_UNUSED TestVector4 default_constructor;

//            auto all_zero = TestVector4::zero();
//            for (size_t i = 0; i < TestVector4::get_size(); ++i) {
//                REQUIRE(is_approx(all_zero[i], 0));
//            }

//            auto all_42 = TestVector4::all(42);
//            for (size_t i = 0; i < TestVector4::get_size(); ++i) {
//                REQUIRE(is_approx(all_42[i], 42));
//            }

//            auto copy = all_42;
//            REQUIRE(is_approx(copy[0], 42));

//            auto moved = std::move(copy);
//            REQUIRE(is_approx(moved[0], 42));

//            TestVector4 swapped;
//            std::swap(moved, swapped);
//            REQUIRE(is_approx(swapped[0], 42));

//            TestVector4 explicit_copy(swapped.data);

//            TestVector4 values(false, 1, 2.f, 3.);
//            REQUIRE(is_approx(values[0], 0));
//            REQUIRE(is_approx(values[1], 1));
//            REQUIRE(is_approx(values[2], 2));
//            REQUIRE(is_approx(values[3], 3));

//            TestVector4 first_two_only(4, 2.);
//            REQUIRE(is_approx(first_two_only[0], 4));
//            REQUIRE(is_approx(first_two_only[1], 2));
//            REQUIRE(is_approx(first_two_only[2], 0));
//            REQUIRE(is_approx(first_two_only[3], 0));
//        }

//        SECTION("Magnitude")
//        {
//            REQUIRE(is_approx(TestVector4(1).get_magnitude_sq(), 1));
//            REQUIRE(is_approx(TestVector4(1).get_magnitude(), 1));
//            REQUIRE(TestVector4(1).is_unit());
//        }
//    }
}
