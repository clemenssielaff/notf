#include "catch2/catch.hpp"

#include "test_app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("graph", "[app][graph]")
{
    SECTION("TheGraph is a Singleton")
    {
        TheGraph& graph = TheGraph::AccessFor<Tester>::get();
        REQUIRE(&graph == &TheGraph::AccessFor<Tester>::get());
    }

    SECTION("Nodes register with the Graph on construction")
    {
//        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 0);

//        TestRootNode root_node;
//        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 0);

//        root_node.create_child<TwoChildrenNode>();
//        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 4);
    }

    SECTION("Nodes in the Graph can be requested by their name")
    {

    }

}
