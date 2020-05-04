#include "catch.hpp"

#include "notf/meta/integer.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("integers", "[meta][integer]") {
    SECTION("fun with digits") {
        REQUIRE(get_digit(123, 0) == 3);
        REQUIRE(get_digit(123, 1) == 2);
        REQUIRE(get_digit(123, 2) == 1);
        REQUIRE(get_digit(123, 3) == 0);

        REQUIRE(count_digits(8) == 1);
        REQUIRE(count_digits(123) == 3);
        REQUIRE(count_digits(1234) == 4);
    }
}
