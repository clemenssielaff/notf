#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("graph", "[app][graph]")
{
    // always reset the graph
    TheGraph::initialize<TestRootNode>();
    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    std::shared_ptr<TestRootNode> root_node;
    { // get the root node as shared_ptr
        NodeHandle root_node_handle = TheGraph::get_root_node();
        REQUIRE(root_node_handle);
        REQUIRE(root_node_handle == TheGraph::get_node(root_node_handle.get_uuid()));
        root_node
            = std::dynamic_pointer_cast<TestRootNode>(NodeHandle::AccessFor<Tester>::get_shared_ptr(root_node_handle));
    }
    REQUIRE(root_node);

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
        SECTION("New Nodes add to the number of children in the graph")
        {
            REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);
            root_node->create_child<TwoChildrenNode>();
            REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 4);
        }

        SECTION("Nodes in the Graph can be requested by their name")
        {
            const std::string test_name = "this_is_a_test_name_indeed";
            NodeHandle leaf_node = root_node->create_child<LeafNodeCT>();
            leaf_node.set_name(test_name);
            REQUIRE(leaf_node.get_name() == test_name);
            REQUIRE(TheGraph::get_node(test_name) == leaf_node);
            REQUIRE(TheGraph::get_node(test_name) == leaf_node);
            REQUIRE(TheGraph::get_node("this_is_not_a_node") != leaf_node);
        }
    }

    SECTION("The Graph can be frozen")
    {
        const auto render_thread_id = make_thread_id(45);

        SECTION("from this thread")
        {
            REQUIRE(!TheGraph::is_frozen());
            {
                NOTF_GUARD(TheGraph::freeze());

                REQUIRE(TheGraph::is_frozen());
                REQUIRE(TheGraph::is_frozen_by(std::this_thread::get_id()));
                REQUIRE(!TheGraph::is_frozen_by(render_thread_id));
            }
            REQUIRE(!TheGraph::is_frozen());
        }

        SECTION("from another thread")
        {
            REQUIRE(!TheGraph::is_frozen());
            {
                NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));

                REQUIRE(TheGraph::is_frozen());
                REQUIRE(TheGraph::is_frozen_by(render_thread_id));
                REQUIRE(!TheGraph::is_frozen_by(std::this_thread::get_id()));
            }
            REQUIRE(!TheGraph::is_frozen());
        }

        SECTION("only once")
        {
            REQUIRE(!TheGraph::is_frozen());
            {
                NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));
                NOTF_GUARD(TheGraph::freeze());

                REQUIRE(TheGraph::is_frozen());
                REQUIRE(TheGraph::is_frozen_by(render_thread_id));
                REQUIRE(!TheGraph::is_frozen_by(std::this_thread::get_id()));
            }
            REQUIRE(!TheGraph::is_frozen());
        }
    }
}
