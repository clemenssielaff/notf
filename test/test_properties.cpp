#include "catch.hpp"

#include "app/property_graph.hpp"
#include "app/window.hpp"
#include "testenv.hpp"

NOTF_USING_NAMESPACE

SCENARIO("a PropertyGraph can be set up and modified", "[app], [property_graph]")
{
    const std::thread::id some_thread_id{1};
    const std::thread::id other_thread_id{2};
    const std::thread::id render_thread_id{3};

    auto& app = Application::instance();
    PropertyGraph& graph = notf_window().property_graph();

    GIVEN("an empty PropertyGraph")
    {
        WHEN("you add properties")
        {
            Property<int> int_prop1(graph, 48);
            Property<int> int_prop2(graph, 2);
            Property<std::string> str_prop1(graph, "derbe");

            THEN("the graph has grown by the number of added properties")
            {
                REQUIRE(PropertyGraph::Access<test::Harness>(graph).size() == 3);
            }

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
                Property<int> int_prop1(graph, 48);
                Property<int> int_prop2(graph, 2);
                Property<std::string> str_prop1(graph, "derbe");
            }
            THEN("the graph is empty again") { CHECK(PropertyGraph::Access<test::Harness>(graph).size() == 0); }
        }

        WHEN("create a graph with a few properties and freeze it")
        {
            std::unique_ptr<Property<int>> iprop = std::make_unique<Property<int>>(graph, 48);
            std::unique_ptr<Property<int>> iprop2 = std::make_unique<Property<int>>(graph, 0);
            std::unique_ptr<Property<std::string>> sprop = std::make_unique<Property<std::string>>(graph, "before");

            PropertyGraph::Access<test::Harness> graph_access(graph);

            graph_access.freeze(render_thread_id);

            AND_THEN("change a value")
            {
                iprop->set_value(24);
                sprop->set_value("after");

                THEN("the new value will replace the old one for all but the render thread")
                {
                    REQUIRE(iprop->value() == 24);
                    REQUIRE(graph_access.read_property(*iprop, some_thread_id) == 24);
                    REQUIRE(graph_access.read_property(*iprop, other_thread_id) == 24);
                    REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 48);

                    REQUIRE(sprop->value() == "after");
                    REQUIRE(graph_access.read_property(*sprop, some_thread_id) == "after");
                    REQUIRE(graph_access.read_property(*sprop, other_thread_id) == "after");
                    REQUIRE(graph_access.read_property(*sprop, render_thread_id) == "before");
                }

                AND_WHEN("the graph is unfrozen again")
                {
                    graph_access.unfreeze();

                    THEN("the new value will replace the old one")
                    {
                        REQUIRE(iprop->value() == 24);
                        REQUIRE(graph_access.read_property(*iprop, some_thread_id) == 24);
                        REQUIRE(graph_access.read_property(*iprop, other_thread_id) == 24);
                        REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 24);

                        REQUIRE(sprop->value() == "after");
                        REQUIRE(graph_access.read_property(*sprop, other_thread_id) == "after");
                        REQUIRE(graph_access.read_property(*sprop, some_thread_id) == "after");
                        REQUIRE(graph_access.read_property(*sprop, render_thread_id) == "after");
                    }
                }
            }

            AND_THEN("delete a property")
            {
                graph_access.delete_property(iprop->node().get(), some_thread_id);

                REQUIRE_THROWS(iprop->value()); // property is actually deleted (not possible without test harness)

                THEN("the render thread will still be able to access the property")
                {
                    REQUIRE(graph_access.read_property(*iprop, render_thread_id) == 48);
                }
                iprop->node() = nullptr; // to avoid double-freeing the node
            }

            AND_THEN("define an expression for a property")
            {
                Property<int>& iprop_ref = *iprop.get();
                iprop2->set_expression([&iprop_ref] { return iprop_ref.value() + 1; }, {iprop_ref});

                THEN("the expression will replace the old value for all but the render thread")
                {
                    REQUIRE(iprop2->value() == 49);
                    REQUIRE(graph_access.read_property(*iprop2, some_thread_id) == 49);
                    REQUIRE(graph_access.read_property(*iprop2, other_thread_id) == 49);
                    REQUIRE(graph_access.read_property(*iprop2, render_thread_id) == 0);
                }

                AND_WHEN("the graph is unfrozen again")
                {
                    graph_access.unfreeze();

                    THEN("the expression will replace the old value")
                    {
                        REQUIRE(iprop2->value() == 49);
                        REQUIRE(graph_access.read_property(*iprop2, some_thread_id) == 49);
                        REQUIRE(graph_access.read_property(*iprop2, other_thread_id) == 49);
                        REQUIRE(graph_access.read_property(*iprop2, render_thread_id) == 49);
                    }
                }
            }

            graph_access.unfreeze(); // always unfreeze!
        }
    }
}
