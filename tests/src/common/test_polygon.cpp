#include "catch2/catch.hpp"

#include "notf/common/geo/polygon.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("polygon", "[common][polygon]") {

    SECTION("Polygon creation") {

        SECTION("Polygons need to contain at least 3 unique vertices") {
            REQUIRE_THROWS_AS(Polygonf({V2f(0, 0), V2f(1, 0)}), LogicError);
            REQUIRE_THROWS_AS(Polygonf({V2f(0, 0), V2f(1, 0), V2f(1, 0)}), LogicError);
        }

        SECTION("Non-unique vertices are folded into one on creation") {
            auto triangle = Polygonf({V2f(0, 0), V2f(0, 1), V2f(0, 1), V2f(1, 0)});
            REQUIRE(triangle.get_vertex_count() == 3);
            REQUIRE(triangle == Polygonf({V2f(0, 0), V2f(0, 1), V2f(1, 0)}));

            auto first_and_last = Polygonf({V2f(0, 0), V2f(0, 1), V2f(1, 0), V2f(0, 0)});
            REQUIRE(first_and_last.get_vertex_count() == 3);
        }
    }

    SECTION("Polygon comparison") {

        SECTION("Simple comparison") {
            auto triangle1 = Polygonf({V2f(0, 0), V2f(1, 1), V2f(1, 0)});
            auto triangle2 = Polygonf({V2f(0, 1), V2f(1, 1), V2f(1, 0)});
            REQUIRE(triangle1 == triangle1);
            REQUIRE(triangle1 != triangle2);
        }

        SECTION("Rotated comparison") {
            auto triangle1 = Polygonf({V2f(0, 0), V2f(1, 1), V2f(1, 0)});
            auto triangle2 = Polygonf({V2f(1, 1), V2f(1, 0), V2f(0, 0)});
            REQUIRE(triangle1 == triangle2);
        }
    }
}
