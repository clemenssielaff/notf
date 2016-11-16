#include "test/catch.hpp"

#include "common/circle.hpp"
using notf::Circle;

SCENARIO("Circles can be tested with other constructs", "[common][circle]")
{
    WHEN("the circle tests if a given point is inside or outside")
    {
        const Circle c1 = {{0.f, 0.f}, 1.f};

        THEN("it will answer correctly")
        {
            REQUIRE(c1.contains({0.f, 0.09999f}));
            REQUIRE(c1.contains({0.f, 1.f}));
            REQUIRE(!c1.contains({0.f, 1.00001f}));
        }
    }

    WHEN("the circle is tested for intersection with another one")
    {
        const Circle c1 = {{0.f, 0.f}, 1.f};

        THEN("the result is correct")
        {
            REQUIRE(c1.intersects({{0.f, 0.f}, 1.f}));
            REQUIRE(c1.intersects({{1.999999f, 0.f}, 1.f}));
            REQUIRE(!c1.intersects({{2.f, 0.f}, 1.f}));
            REQUIRE(!c1.intersects({{2.00001f, 0.f}, 1.f}));
        }
    }
}
