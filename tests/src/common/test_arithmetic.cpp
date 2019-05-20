#include "catch.hpp"

#include <iostream>

#include "notf/common/geo/matrix3.hpp"

#include "test/utils.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Matrix3", "[common][arithmetic][geo][matrix3]") {

    SECTION("default construction") { NOTF_UNUSED M3d default_constructed; }

    SECTION("translation") {
        REQUIRE(V2d::zero() * M3d::translation(0, 0) == V2d::zero());

        const auto random_v2 = random_tested<V2d>();
        REQUIRE(random_v2 * M3d::translation(0, 0) == random_v2);

        const V2d example{123, 345};
        REQUIRE(example * M3d::translation(7, 15) == V2d(130, 360));

        const auto random_translation = random_tested<V2d>();
        REQUIRE((random_v2 * M3d::translation(random_translation) * M3d::translation(-random_translation))
                    .is_approx(random_v2));
    }
}
