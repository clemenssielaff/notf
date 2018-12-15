#include "catch2/catch.hpp"

#include "test/app.hpp"
#include "test/utils.hpp"

NOTF_USING_NAMESPACE;

using GraphAccess = detail::Graph::AccessFor<Tester>;

SCENARIO("graph", "[app][graph]") {
    TheApplication app(TheApplication::Arguments{});
    auto root_node = TheRootNode();

    SECTION("Nodes register with the Graph on construction") {
        SECTION("New Nodes add to the number of children in the graph") {
            REQUIRE(TheGraph()->get_node_count() == 1);
            root_node.create_child<TestNode>();
            root_node.create_child<TestNode>();
            REQUIRE(TheGraph()->get_node_count() == 3);
        }

        SECTION("Nodes in the Graph can be requested by their name") {
            const std::string test_name = "this_is_a_test_name_indeed";
            NodeHandle leaf_node = root_node.create_child<TestNode>();
            leaf_node->set_name(test_name);
            REQUIRE(leaf_node.get_name() == test_name);
            REQUIRE(TheGraph()->get_node(test_name) == leaf_node);
            REQUIRE(TheGraph()->get_node("this_is_not_a_node") != leaf_node);
        }

        SECTION("Nodes in the Graph can be requested by their unique Uuid") {
            auto node = root_node.create_child<TestNode>().to_handle();
            REQUIRE(TheGraph()->get_node(node.get_uuid()) == node);
            REQUIRE(!TheGraph()->get_node(Uuid()));

            auto evil_node = root_node.create_child<TestNode>().to_handle();
            Node::AccessFor<Tester>(evil_node).set_uuid(node.get_uuid());
            REQUIRE_THROWS_AS(GraphAccess::register_node(evil_node), NotUniqueError);
        }

        SECTION("Nodes can be named and renamed") {
            auto node = root_node.create_child<TestNode>().to_handle();
            node->set_name("SuperName3000");
            REQUIRE(TheGraph()->get_node("SuperName3000") == node);

            node->set_name("SuperAwesomeName4000Pro");
            REQUIRE(TheGraph()->get_node("SuperAwesomeName4000Pro") == node);
        }

        SECTION("Node names are unique") {
            SECTION("duplicates have a ostfix added to their name") {
                auto original = root_node.create_child<TestNode>().to_handle();
                original->set_name("Connor MacLeod");

                auto impostor = root_node.create_child<TestNode>().to_handle();
                impostor->set_name("Connor MacLeod");

                REQUIRE(TheGraph()->get_node("Connor MacLeod") == original);
                REQUIRE(TheGraph()->get_node("Connor MacLeod_02") == impostor);
            }

            SECTION("names of expired nodes are available") {
                {
                    auto original = root_node.create_child<TestNode>().to_owner();
                    original->set_name("Bob");
                }
                auto next_original = root_node.create_child<TestNode>().to_owner();
                next_original->set_name("Bob");
            }
        }
    }
}
