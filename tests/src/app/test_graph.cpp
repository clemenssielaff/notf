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

    SECTION("The Grah can be locked from anywhere")
    {
        {
            REQUIRE(!TheGraph::is_locked_by_this_thread());
        }
        {
            NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
            REQUIRE(TheGraph::is_locked_by_this_thread());
        }
    }

    SECTION("Nodes register with the Graph on construction")
    {
        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1); // old root node

        TheGraph::initialize<TestRootNode>();
        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1); // new root node
        NodeHandle root_node = TheGraph::get_root_node();
        REQUIRE(root_node);
        REQUIRE(root_node == TheGraph::get_node(root_node.get_uuid()));

        NodePtr root_node_ptr = NodeHandle::AccessFor<Tester>::get_shared_ptr(root_node);
        auto test_root_node = std::dynamic_pointer_cast<TestRootNode>(root_node_ptr);
        REQUIRE(test_root_node);

        test_root_node->create_child<TwoChildrenNode>();
        REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 4);
    }

    SECTION("Nodes in the Graph can be requested by their name") {}
}
