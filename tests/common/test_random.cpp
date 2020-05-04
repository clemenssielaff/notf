#include "catch.hpp"

#include "notf/common/geo/vector2.hpp"
#include "notf/common/random.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("random generators", "[common][random]") {

    SECTION("random scalar") {
        SECTION("random int") {
            static_assert(std::is_same_v<decltype(random<int>(0, 100)), int>);
            const int value = random<int>(0, 100);
            REQUIRE(value >= 0);
            REQUIRE(value <= 100);
        }
        SECTION("random double") {
            static_assert(std::is_same_v<decltype(random<double>(0, 100)), double>);
            const double value = random<double>(0, 100);
            REQUIRE(value >= 0);
            REQUIRE(value <= 100);
        }
    }

    SECTION("random arithmetic") {
        SECTION("random vector2") {
            static_assert(std::is_same_v<decltype(random<V2f>(0, 100)), V2f>);
            const V2f value = random<V2f>(0, 100);
            REQUIRE(value.x() >= 0);
            REQUIRE(value.x() < 100);
            REQUIRE(value.y() >= 0);
            REQUIRE(value.y() < 100);
        }
    }

    SECTION("random boolean") {
        static_assert(std::is_same_v<decltype(random<bool>()), bool>);
        const bool value = random<bool>();
        REQUIRE((value == true || value == false));
    }
}
