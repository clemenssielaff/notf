#include "catch.hpp"

#include "notf/common/geo/polyline.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("polyline", "[common][polyline]") {

//    SECTION("Polyline creation") {

//        SECTION("Polylines need to contain at least 3 unique vertices") {
//            REQUIRE_THROWS_AS(Polylinef({V2f(0, 0), V2f(1, 0)}), LogicError);
//            REQUIRE_THROWS_AS(Polylinef({V2f(0, 0), V2f(1, 0), V2f(1, 0)}), LogicError);
//        }

//        SECTION("Non-unique vertices are folded into one on creation") {
//            auto triangle = Polylinef({V2f(0, 0), V2f(0, 1), V2f(0, 1), V2f(1, 0)});
//            REQUIRE(triangle.get_size() == 3);
//            REQUIRE(triangle == Polylinef({V2f(0, 0), V2f(0, 1), V2f(1, 0)}));

//            auto first_and_last = Polylinef({V2f(0, 0), V2f(0, 1), V2f(1, 0), V2f(0, 0)});
//            REQUIRE(first_and_last.get_size() == 3);
//        }
//    }

    SECTION("Polyline comparison") {

        SECTION("Simple comparison") {
            auto triangle1 = Polylinef({V2f(0, 0), V2f(1, 1), V2f(1, 0)});
            auto triangle2 = Polylinef({V2f(0, 1), V2f(1, 1), V2f(1, 0)});
            REQUIRE(triangle1 == triangle1);
            REQUIRE(triangle1 != triangle2);
        }

//        SECTION("Rotated comparison") {
//            auto triangle1 = Polylinef({V2f(0, 0), V2f(1, 1), V2f(1, 0)});
//            auto triangle2 = Polylinef({V2f(1, 1), V2f(1, 0), V2f(0, 0)});
//            REQUIRE(triangle1 == triangle2);
//        }
    }
}
