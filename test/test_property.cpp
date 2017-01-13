#include "test/catch.hpp"

#include <memory>

#include "core/properties.hpp"
using namespace notf;

SCENARIO("Properties within a single PropertyMap", "[core][properties]")
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

SCENARIO("Properties in two different PropertyMaps", "[core][properties]")
{
    PropertyMap left;
    std::unique_ptr<PropertyMap> right = std::make_unique<PropertyMap>();
    IntProperty* left_a = left.create_property<IntProperty>("left_a", 2);
    IntProperty* left_b = left.create_property<IntProperty>("left_b", 7);
    IntProperty* right_a = right->create_property<IntProperty>("right_a", 3);
    IntProperty* right_b = right->create_property<IntProperty>("right_b", 5);

    WHEN("an expression contains Properties of different PropertyMaps")
    {
        property_expression(left_a, {
            return left_b->get_value() + right_a->get_value() + right_b->get_value();
        }, left_b, right_a, right_b);

        THEN("it will work just fine")
        {
            REQUIRE(left_a->has_expression());
            REQUIRE(left_a->get_value() == 15);
        }

        WHEN("one of the PropertyMaps is deleted")
        {
            right.reset();

            THEN("the dependent Properties drop their expressions but don't change their value")
            {
                REQUIRE(!left_a->has_expression());
                REQUIRE(left_a->get_value() == 15);
            }
        }
    }
}
