#include "test/catch.hpp"

#include "core/properties.hpp"
using namespace notf;

SCENARIO("Properties", "[core][properties]")
{
    PropertyMap map;
    FloatProperty* one = map.create_property<FloatProperty>("one", 1.2);
    IntProperty* two = map.create_property<IntProperty>("two", 2);
    IntProperty* three = map.create_property<IntProperty>("three", 3);
    int four = 4;

    WHEN("a Property is created with a value")
    {
        THEN("the value will be stored in it")
        {
            REQUIRE(one->get_value() == approx(1.2f));
            REQUIRE(two->get_value() == 2);
            REQUIRE(three->get_value() == 3);
        }
    }

    WHEN("a Property is requested from the map")
    {
        THEN("requesting the property with the right type will yield the correct result")
        {
            REQUIRE(map.get<FloatProperty>("one")->get_value() == approx(1.2f));
        }

        THEN("requesting the property with the wrong type will throw a std::runtime_error")
        {
            REQUIRE_THROWS_AS(map.get<IntProperty>("one"), std::runtime_error);
        }

        THEN("requesting a property with an unknown name will throw a std::out_of_range")
        {
            REQUIRE_THROWS_AS(map.get<FloatProperty>("one_million"), std::out_of_range);
        }
    }

    WHEN("an expression is created")
    {
        property_expression(one, {
            return two->get_value() + three->get_value() + four;
        }, two, three, four);

        THEN("it is evaluated immediately")
        {
            REQUIRE(one->get_value() == approx(9));
            REQUIRE(two->get_value() == 2);
            REQUIRE(three->get_value() == 3);
            REQUIRE(four == 4);
        }

        AND_WHEN("one of the dependent properties change")
        {
            two->set_value(12);

            THEN("the target will update")
            {
                REQUIRE(one->get_value() == approx(19));
                REQUIRE(two->get_value() == 12);
                REQUIRE(three->get_value() == 3);
                REQUIRE(four == 4);
            }
        }

        AND_WHEN("a cyclic expression is generated")
        {
            THEN("an std::runtime_error is thrown immediately")
            {
                REQUIRE_THROWS_AS(property_expression(two, {
                    return one->get_value();
                }, one), std::runtime_error);
            }
        }
    }
}
