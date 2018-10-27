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
    std::shared_ptr<TestRootNode> root_node = std::static_pointer_cast<TestRootNode>(to_shared_ptr(root_node_handle));
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
        NodeHandle two_child_node = root_node->create_child<TwoChildrenNode>();
        NodeHandle first_child = two_child_node.get_child(0);
        REQUIRE(first_child.get_first_ancestor<TwoChildrenNode>() == two_child_node);
        REQUIRE(first_child.get_first_ancestor<TestRootNode>() == root_node_handle);
        REQUIRE(first_child.get_first_ancestor<SingleChildNode>().is_expired());

        REQUIRE(first_child.has_ancestor(two_child_node));
        REQUIRE(first_child.has_ancestor(root_node_handle));
        REQUIRE(!first_child.has_ancestor(two_child_node.get_child(1)));
        REQUIRE(!first_child.has_ancestor(NodeHandle()));
    }

    SECTION("Nodes have user definable flags") { REQUIRE(Node::get_user_flag_count() > 0); }

    SECTION("Compile Time Nodes have Compile Time Properties")
    {
        auto node = root_node->create_child<LeafNodeCT>().to_owner();
        PropertyHandle<int> int_prop = node.get_property<int>("int");
        REQUIRE(!int_prop.is_expired());

        SECTION("compile time and run time acquired Properties are the same")
        {
            PropertyHandle<int> int_prop_rt = node.get_property("int"_id);
            REQUIRE(!int_prop_rt.is_expired());
            REQUIRE(int_prop_rt == int_prop);
        }

        SECTION("Compile Time Nodes can be asked for non-existing run time Properies")
        {
            REQUIRE_THROWS_AS(node.get_property<int>("float").is_expired(), Node::NoSuchPropertyError);
            REQUIRE_THROWS_AS(node.get_property<float>("int").is_expired(), Node::NoSuchPropertyError);
            REQUIRE_THROWS_AS(node.get_property<float>("not a property name").is_expired(), Node::NoSuchPropertyError);
        }

        SECTION("two of the same Properties of different Nodes are not equal")
        {
            TypedNodeHandle<LeafNodeCT> node2 = root_node->create_child<LeafNodeCT>();
            PropertyHandle<int> int_prop_2 = node2.get_property<int>("int");
            REQUIRE(!int_prop_2.is_expired());
            REQUIRE(int_prop != int_prop_2);
        }

        SECTION("you can change the property hash by changing any property value")
        {
            const size_t property_hash = Node::AccessFor<Tester>(node).get_property_hash();
            int_prop.set(int_prop.get() + 1);
            REQUIRE(property_hash != Node::AccessFor<Tester>(node).get_property_hash());
        }
    }

    SECTION("RunTimeNodes create their Properties in the constructor")
    {
        SECTION("Property names have to be unique")
        {
            struct NotUniquePropertyNode : public RunTimeNode {
                NOTF_UNUSED NotUniquePropertyNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
                {
                    _create_property<int>("not_unique", 0, true);
                    _create_property<int>("not_unique", 6587, false);
                }
            };
            REQUIRE_THROWS_AS(root_node->create_child<NotUniquePropertyNode>(), NotUniqueError);
        }

        SECTION("Properties can only be created in the constructor")
        {
            struct FinalizedNode : public RunTimeNode {
                NOTF_UNUSED FinalizedNode(valid_ptr<Node*> parent) : RunTimeNode(parent)
                {
                    _create_property<int>("int", 0, true);
                }

                void fail()
                {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    _create_property<int>("won't work because I'm finalized", 0, true);
                }
            };
            auto node_handle = root_node->create_child<FinalizedNode>().to_handle();
            auto node_ptr = to_shared_ptr(node_handle);
            REQUIRE_THROWS_AS(node_ptr->fail(), Node::FinalizedError);
        }
    }

    SECTION("Run Time Nodes have Run Time Properties")
    {
        NodeHandle node = root_node->create_child<LeafNodeRT>();

        SECTION("can be asked fo non-existing Properties")
        {
            REQUIRE(!node.get_property<float>("float").is_expired());
            REQUIRE_THROWS_AS(node.get_property<float>("not a property").is_expired(), Node::NoSuchPropertyError);
            REQUIRE_THROWS_AS(node.get_property<bool>("float").is_expired(), Node::NoSuchPropertyError);
        }

        SECTION("you can change the property hash by changing any property value")
        {
            PropertyHandle<float> prop = node.get_property<float>("float");

            const size_t property_hash = Node::AccessFor<Tester>(node).get_property_hash();
            prop.set(prop.get() + 1.f);
            REQUIRE(property_hash != Node::AccessFor<Tester>(node).get_property_hash());
        }
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
            REQUIRE(int_property.get() == 321);
        }
        REQUIRE(int_property.get() == 321);
    }

    SECTION("Node Handles will not crash when you try call a method on them when they are expired")
    {
        NodeHandle expired;
        {
            auto owner = root_node->create_child<LeafNodeCT>().to_owner();
            expired = owner;
            REQUIRE(!expired.is_expired());
        }
        REQUIRE(expired.is_expired());

        REQUIRE_THROWS_AS(expired.stack_back(), HandleExpiredError); // mutable
        REQUIRE_THROWS_AS(expired.is_in_back(), HandleExpiredError); // const

        // TODO: test all node handle functions
    }

    SECTION("Two Nodes should have at least the root node as a common ancestor")
    {
        auto two_child_parent_handle = root_node->create_child<TwoChildrenNode>().to_owner();
        auto two_child_parent_ptr = to_shared_ptr(two_child_parent_handle);
        TypedNodeHandle<LeafNodeRT> first_child_handle = two_child_parent_ptr->first_child;
        TypedNodeHandle<LeafNodeRT> second_child_handle = two_child_parent_ptr->second_child;

        REQUIRE(first_child_handle.get_common_ancestor(second_child_handle) == two_child_parent_handle);

        std::shared_ptr<TestRootNode> second_root = std::make_shared<TestRootNode>();
        NodeHandle foreign_node = second_root->create_child<LeafNodeCT>();
        REQUIRE_THROWS_AS(first_child_handle.get_common_ancestor(foreign_node), Node::HierarchyError);
    }
}
