#include "catch.hpp"

#include "app/property_graph.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

SCENARIO("a PropertyGraph can be set up and modified", "[app], [property_graph]")
{
    auto& app = Application::instance();

    GIVEN("an empty PropertyGraph")
    {
        WHEN("you add properties")
        {
            Property<int> int_prop1(48);
            Property<int> int_prop2(2);
            Property<std::string> str_prop1("derbe");

            THEN("the graph has grown by the number of added properties") { REQUIRE(app.property_graph().size() == 3); }

            THEN("you can read their value")
            {
                REQUIRE(int_prop1.value() == 48);
                REQUIRE(int_prop2.value() == 2);
                REQUIRE(str_prop1.value() == "derbe");
            }

            WHEN("you change the values")
            {
                int_prop1.set_value(24);
                int_prop2.set_value(16);
                str_prop1.set_value("ultraderbe");

                THEN("you can read the changed values")
                {
                    REQUIRE(int_prop1.value() == 24);
                    REQUIRE(int_prop2.value() == 16);
                    REQUIRE(str_prop1.value() == "ultraderbe");
                }
            }

            WHEN("you set an expression")
            {
                int_prop1.set_expression([&int_prop2]() -> int { return int_prop2.value() + 7; }, {int_prop2});

                THEN("you can read the expression's result")
                {
                    REQUIRE(int_prop1.value() == 9);
                    REQUIRE(int_prop2.value() == 2);
                    REQUIRE(str_prop1.value() == "derbe");
                }
            }
        }

        WHEN("all properties have gone out of scope")
        {
            {
                Property<int> int_prop1(48);
                Property<int> int_prop2(2);
                Property<std::string> str_prop1("derbe");
            }
            THEN("the graph is empty again") { CHECK(app.property_graph().size() == 0); }
        }

        WHEN("you freeze the graph")
        {
            Property<int> int_prop1(48);

            PropertyGraph::Access<test::Test> graph_access;
            graph_access.freeze(1);

            AND_THEN("change a value before unfreezing")
            {
                int_prop1.set_value(24);

                THEN("the new value will replace the old one for all but the render thread")
                {
                    REQUIRE(int_prop1.value() == 24);
                }

                AND_WHEN("the graph is unfrozen again")
                {
                    graph_access.unfreeze();

                    THEN("the new value will replace the old one") { REQUIRE(int_prop1.value() == 24); }
                }
            }
        }
    }
}
