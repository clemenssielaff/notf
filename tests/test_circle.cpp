#include "catch.hpp"

#include "common/circle.hpp"
using notf::Circlef;

TEST_CASE("Circles can be tested with other constructs", "[common][circle]")
{
    SECTION("circle/point")
    {
        const Circlef c1 = {{0.f, 0.f}, 1.f};

        REQUIRE(c1.contains({0.f, 0.09999f}));
        REQUIRE(c1.contains({0.f, 1.f}));
        REQUIRE(!c1.contains({0.f, 1.00001f}));
    }

    SECTION("circle/circle")
    {
        const Circlef c1 = {{0.f, 0.f}, 1.f};

        REQUIRE(c1.intersects({{0.f, 0.f}, 1.f}));
        REQUIRE(c1.intersects({{1.999999f, 0.f}, 1.f}));
        REQUIRE(!c1.intersects({{2.f, 0.f}, 1.f}));
        REQUIRE(!c1.intersects({{2.00001f, 0.f}, 1.f}));
    }
}
