#include "catch2/catch.hpp"

#include "test_app.hpp"
#include "test_utils.hpp"

#include "notf/app/node.hpp"

NOTF_USING_NAMESPACE;
NOTF_USING_LITERALS;

SCENARIO("Nodes can limit what kind of children or parent types they can have", "[app][node]")
{
    struct NodeA : RunTimeNode {
        NOTF_UNUSED NodeA(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    struct NodeB : RunTimeNode {
        NOTF_UNUSED NodeB(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    struct OnlyParentB : RunTimeNode {
        using allowed_child_types = std::tuple<NodeB>;
        NOTF_UNUSED OnlyParentB(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    struct DoNotParentB : RunTimeNode {
        using forbidden_child_types = std::tuple<NodeB>;
        NOTF_UNUSED DoNotParentB(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    struct OnlyChildB : RunTimeNode {
        using allowed_parent_types = std::tuple<NodeB>;
        NOTF_UNUSED OnlyChildB(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    struct DoNotChildB : RunTimeNode {
        using forbidden_parent_types = std::tuple<NodeB>;
        NOTF_UNUSED DoNotChildB(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
    };

    // both must be derived from Node
    REQUIRE(!detail::can_node_parent<NodeA, float>());
    REQUIRE(!detail::can_node_parent<bool, NodeB>());
    REQUIRE(detail::can_node_parent<NodeA, NodeB>());

    // if A has a list of explicitly allowed child types, B must be in it
    REQUIRE(!detail::can_node_parent<OnlyParentB, NodeA>());
    REQUIRE(detail::can_node_parent<OnlyParentB, NodeB>());

    // ... otherwise, if A has a list of explicitly forbidden child types, B must NOT be in it
    REQUIRE(detail::can_node_parent<DoNotParentB, NodeA>());
    REQUIRE(!detail::can_node_parent<DoNotParentB, NodeB>());

    // if B has a list of explicitly allowed parent types, A must be in it
    REQUIRE(!detail::can_node_parent<NodeA, OnlyChildB>());
    REQUIRE(detail::can_node_parent<NodeB, OnlyChildB>());

    // ... otherwise, if B has a list of explicitly forbidden parent types, A must NOT be in it
    REQUIRE(detail::can_node_parent<NodeA, DoNotChildB>());
    REQUIRE(!detail::can_node_parent<NodeB, DoNotChildB>());
}

SCENARIO("Nodes have Properties", "[app][node][property]")
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

    SECTION("Compile Time Node")
    {
        TypedNodeHandle<LeafNodeCT> test_node = root_node->create_child<LeafNodeCT>();

        PropertyHandle<float> float_handle_rt = test_node.get_property<float>("float");
        REQUIRE(!float_handle_rt.is_expired());

        PropertyHandle<float> float_handle_ct = test_node.get_property("float"_id);
        REQUIRE(!float_handle_ct.is_expired());

        REQUIRE(float_handle_ct == float_handle_rt);
    }
}

SCENARIO("Nodes can create child nodes", "[app][node]")
{
    SECTION("Compile Time Nodes can be identified")
    {
        REQUIRE(detail::is_compile_time_node_v<LeafNodeCT>);
        REQUIRE(!detail::is_compile_time_node_v<LeafNodeRT>);
    }
}
