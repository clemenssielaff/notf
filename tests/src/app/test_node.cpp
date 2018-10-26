#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
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

SCENARIO("Basic Node Setup", "[app][node][property]")
{
    // always reset the graph
    TheGraph::initialize<TestRootNode>();
    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    NodeHandle root_node_handle = TheGraph::get_root_node();
    std::shared_ptr<TestRootNode> root_node;
    { // get the root node as shared_ptr
        REQUIRE(root_node_handle);
        REQUIRE(root_node_handle == TheGraph::get_node(root_node_handle.get_uuid()));
        root_node
            = std::dynamic_pointer_cast<TestRootNode>(NodeHandle::AccessFor<Tester>::get_shared_ptr(root_node_handle));
    }
    REQUIRE(root_node);

    SECTION("the nested 'NewNode' type takes care of casting to a NodeOwner or -Handle of the right type")
    {
        SECTION("NodeOwners can only be created once")
        {
            auto new_node = root_node->create_child<LeafNodeCT>();
            TypedNodeOwner<LeafNodeCT> owner1 = new_node;
            REQUIRE_THROWS_AS(static_cast<TypedNodeOwner<LeafNodeCT>>(new_node), ValueError);
        }
    }

    SECTION("Nodes can create child Nodes")
    {
        SECTION("and count them")
        {
            REQUIRE(root_node_handle.get_child_count() == 0);
            NodeHandle new_node = root_node->create_child<LeafNodeCT>();
            REQUIRE(root_node_handle.get_child_count() == 1);
            REQUIRE(new_node.get_child_count() == 0);
        }

        SECTION("but only on themselves")
        {
            class SchlawinerNode : public RunTimeNode {
            public:
                NOTF_UNUSED SchlawinerNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
                void be_naughty()
                {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    _create_child<LeafNodeCT>(_get_parent());
                }
            };

            TypedNodeHandle<SchlawinerNode> node_handle = root_node->create_child<SchlawinerNode>();
            auto node = std::dynamic_pointer_cast<SchlawinerNode>(
                TypedNodeHandle<SchlawinerNode>::AccessFor<Tester>::get_shared_ptr(node_handle));
            REQUIRE(node);
            REQUIRE_THROWS_AS(node->be_naughty(), InternalError);
        }
    }

    SECTION("Nodes can inspect their hierarchy")
    {
        NodeHandle single_child_node = root_node->create_child<SingleChildNode>();
        NodeHandle leaf_node = single_child_node.get_child(0);
        REQUIRE(leaf_node.get_first_ancestor<SingleChildNode>() == single_child_node);
        REQUIRE(leaf_node.get_first_ancestor<TestRootNode>() == root_node_handle);
        REQUIRE(leaf_node.get_first_ancestor<TwoChildrenNode>().is_expired());
    }

    SECTION("Nodes have user definable flags") { REQUIRE(Node::get_user_flag_count() > 0); }

    SECTION("Compile Time Nodes have Compile Time Properties")
    {
        auto test_node1 = root_node->create_child<LeafNodeCT>().to_owner();

        PropertyHandle<float> float_handle1_rt = test_node1.get_property<float>("float");
        REQUIRE(!float_handle1_rt.is_expired());

        PropertyHandle<float> float_handle_ct = test_node1.get_property("float"_id);
        REQUIRE(!float_handle_ct.is_expired());

        REQUIRE(float_handle_ct == float_handle1_rt);

        TypedNodeHandle<LeafNodeCT> test_node2 = root_node->create_child<LeafNodeCT>();
        PropertyHandle<float> float_handle2_rt = test_node2.get_property<float>("float");
        REQUIRE(!float_handle2_rt.is_expired());
        REQUIRE(float_handle1_rt != float_handle2_rt);
    }

    SECTION("Run Time Nodes have Run Time Properties")
    {
        NodeHandle node = root_node->create_child<LeafNodeRT>();

        REQUIRE(!node.get_property<float>("float").is_expired());
        REQUIRE_THROWS_AS(node.get_property<float>("not a property").is_expired(), Node::NoSuchPropertyError);
        REQUIRE_THROWS_AS(node.get_property<bool>("float").is_expired(), Node::NoSuchPropertyError);
    }

    SECTION("whenever a Property changes, its Node is marked dirty")
    {
        TypedNodeHandle<LeafNodeCT> node_handle = root_node->create_child<LeafNodeCT>();
        auto node = std::dynamic_pointer_cast<LeafNodeCT>(
            TypedNodeHandle<LeafNodeCT>::AccessFor<Tester>::get_shared_ptr(node_handle));
        REQUIRE(node);

        REQUIRE(!Node::AccessFor<Tester>(*node.get()).is_dirty());
        auto property = node_handle.get_property("float"_id);
        property.set(23);
        REQUIRE(Node::AccessFor<Tester>(*node.get()).is_dirty());
    }

    SECTION("Nodes create a copy of their data to modify, if the graph is frozen")
    {
        const auto render_thread_id = make_thread_id(45);
        auto node = root_node->create_child<LeafNodeCT>().to_handle();
        auto int_property = node.get_property("int"_id);
        REQUIRE(int_property.get() == 123);
        {
            NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));
            int_property.set(321);
        }
    }
}
