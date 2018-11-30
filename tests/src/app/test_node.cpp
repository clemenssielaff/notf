#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/node.hpp"
#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

#ifdef NOTF_MSVC
static constexpr StringConst int_id_name = "int";
static constexpr StringConst float_id_name = "float";
static constexpr auto int_id = make_string_type<int_id_name>();
static constexpr auto float_id = make_string_type<float_id_name>();
#else
constexpr auto int_id = "int"_id;
constexpr auto float_id = "float"_id;
#endif

SCENARIO("Nodes can limit what kind of children or parent types they can have", "[app][node]") {
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

SCENARIO("Basic Node Setup", "[app][node][property]") {
    // always reset the graph
    TheGraph::AccessFor<Tester>::reset();
    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    NodeHandle root_node_handle = TheGraph::get_root_node();
    RootNodePtr root_node_ptr = std::static_pointer_cast<RootNode>(to_shared_ptr(root_node_handle));
    REQUIRE(root_node_ptr);
    auto root_node = Node::AccessFor<Tester>(*root_node_ptr);

    const auto render_thread_id = make_thread_id(45);

    SECTION("the nested 'NewNode' type takes care of casting to a NodeOwner or -Handle of the right type") {
        SECTION("NodeOwners can only be created once") {
            auto new_node = root_node.create_child<LeafNodeCT>();
            TypedNodeOwner<LeafNodeCT> owner1 = new_node.to_owner();
            REQUIRE_THROWS_AS(new_node.to_owner(), ValueError);
        }
    }

    SECTION("Nodes can create child Nodes") {
        SECTION("and count them") {
            REQUIRE(root_node_handle.get_child_count() == 0);
            NodeHandle new_node = root_node.create_child<LeafNodeCT>();
            REQUIRE(root_node_handle.get_child_count() == 1);
            REQUIRE(new_node.get_child_count() == 0);
        }

        SECTION("but only on themselves") {
            class SchlawinerNode : public RunTimeNode {
            public:
                NOTF_UNUSED SchlawinerNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {}
                void be_naughty() {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    _create_child<LeafNodeCT>(to_shared_ptr(get_parent()).get());
                }
            };

            TypedNodeHandle<SchlawinerNode> node_handle = root_node.create_child<SchlawinerNode>();
            auto node = std::dynamic_pointer_cast<SchlawinerNode>(
                TypedNodeHandle<SchlawinerNode>::AccessFor<Tester>::get_shared_ptr(node_handle));
            REQUIRE(node);
            REQUIRE_THROWS_AS(node->be_naughty(), InternalError);
        }
    }

    SECTION("Nodes can inspect their hierarchy") {
        NodeHandle two_child_node = root_node.create_child<TwoChildrenNode>();
        NodeHandle first_child = two_child_node.get_child(0);
        NodeHandle second_child = two_child_node.get_child(1);

        REQUIRE(first_child.get_parent() == two_child_node);

        REQUIRE(first_child.get_first_ancestor<TwoChildrenNode>() == two_child_node);
        REQUIRE(first_child.get_first_ancestor<RootNode>() == root_node_handle);
        REQUIRE(first_child.get_first_ancestor<TestNode>().is_expired());

        REQUIRE(first_child.has_ancestor(two_child_node));
        REQUIRE(first_child.has_ancestor(root_node_handle));
        REQUIRE(!first_child.has_ancestor(second_child));
        REQUIRE(!first_child.has_ancestor(NodeHandle()));
        REQUIRE(!Node::AccessFor<Tester>(first_child).has_ancestor(nullptr)); // not accessible using API

        REQUIRE(first_child.get_common_ancestor(second_child) == two_child_node);
        REQUIRE(first_child.get_common_ancestor(NodeHandle()).is_expired());

        REQUIRE_THROWS_AS(two_child_node.get_child(1000), OutOfBounds);
    }

    SECTION("Nodes can modify their hierarchy") {
        SECTION("remove a child") {
            class RemoveChildNode : public RunTimeNode {
            public:
                NOTF_UNUSED RemoveChildNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    first_child = _create_child<LeafNodeRT>(this);
                }
                NOTF_UNUSED void remove_child() {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    first_child = {};
                }
                NodeOwner first_child;
            };

            auto node = root_node.create_child<RemoveChildNode>().to_handle();

            { // these are not real functions, you will never get to them through the API alone
                NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                Node::AccessFor<Tester>(node).remove_child(NodeHandle());     // ignored
                Node::AccessFor<Tester>(node).remove_child(root_node_handle); // ignored
            }

            REQUIRE(node.get_child_count() == 1);
            to_shared_ptr(node)->remove_child();
            REQUIRE(node.get_child_count() == 0);
        }

        SECTION("add a child") {
            auto node1 = root_node.create_child<TestNode>().to_owner();
            REQUIRE(node1.get_child_count() == 0);
            {
                NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));

                to_shared_ptr(node1)->create_child<TestNode>();
                to_shared_ptr(node1)->create_child<TestNode>();
                REQUIRE(node1.get_child_count() == 2);

                REQUIRE(Node::AccessFor<Tester>(node1).get_child_count(render_thread_id) == 0);
            }
            REQUIRE(node1.get_child_count() == 2);
        }

        SECTION("Change a parent") {
            auto node1 = root_node.create_child<TestNode>().to_owner();
            auto node2 = root_node.create_child<TestNode>().to_owner();

            auto child1 = to_shared_ptr(node1)->create_child<TestNode>().to_owner();
            REQUIRE(node1.get_child_count() == 1);
            REQUIRE(node2.get_child_count() == 0);

            Node::AccessFor<Tester>(child1).set_parent(node2);
            REQUIRE(node1.get_child_count() == 0);
            REQUIRE(node2.get_child_count() == 1);

            Node::AccessFor<Tester>(child1).set_parent(node2);
            REQUIRE(node1.get_child_count() == 0);
            REQUIRE(node2.get_child_count() == 1);

            Node::AccessFor<Tester>(child1).set_parent(NodeHandle()); // ignored
            REQUIRE(node1.get_child_count() == 0);
            REQUIRE(node2.get_child_count() == 1);

            REQUIRE(child1.get_parent() == node2);
            {
                NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));

                Node::AccessFor<Tester>(child1).set_parent(node1);
                REQUIRE(child1.get_parent() == node1);

                REQUIRE(Node::AccessFor<Tester>(child1).get_parent(render_thread_id) == node2);
            }
            REQUIRE(child1.get_parent() == node1);
        }
    }

    SECTION("Nodes have a default name") {
        auto node1 = root_node.create_child<LeafNodeCT>().to_owner();
        auto node2 = root_node.create_child<LeafNodeRT>().to_handle();

        // there is actually a 1 : 100^4 chance that this may fail, but in that case, just do it again
        REQUIRE(node1.get_default_name() != node2.get_default_name());
    }

    SECTION("Nodes have user definable flags") {
        enum UserFlags { FIRST, OUT_OF_BOUNDS = Node::AccessFor<Tester>::get_user_flag_count() + 1 };
        NodeHandle node_handle = root_node.create_child<TestNode>();
        TestNode& node = *(std::static_pointer_cast<TestNode>(to_shared_ptr(node_handle)));

        REQUIRE(Node::AccessFor<Tester>::get_user_flag_count() > 0);

        REQUIRE(!node.get_flag(FIRST));
        node.set_flag(FIRST);
        REQUIRE(node.get_flag(FIRST));

        REQUIRE_THROWS_AS(node.get_flag(OUT_OF_BOUNDS), OutOfBounds);
        REQUIRE_THROWS_AS(node.set_flag(OUT_OF_BOUNDS), OutOfBounds);
    }

    SECTION("User definable flags are frozen with the Graph") {
        NodeHandle node_handle = root_node.create_child<TestNode>();
        TestNode& node = *(std::static_pointer_cast<TestNode>(to_shared_ptr(node_handle)));

        REQUIRE(!node.get_flag(0));
        {
            NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));

            node.set_flag(0);
            REQUIRE(node.get_flag(0));

            REQUIRE(!Node::AccessFor<Tester>(node).get_flag(0, render_thread_id));
        }
        REQUIRE(node.get_flag(0));
    }

    SECTION("Nodes have a z-order") {
        NodeHandle three_child_node = root_node.create_child<ThreeChildrenNode>();
        NodeHandle first = three_child_node.get_child(0);
        NodeHandle second = three_child_node.get_child(1);
        NodeHandle third = three_child_node.get_child(2);

        SECTION("The z-order can be queried") {
            REQUIRE(!first.is_in_front());
            REQUIRE(!second.is_in_front());
            REQUIRE(third.is_in_front());

            REQUIRE(first.is_in_back());
            REQUIRE(!second.is_in_back());
            REQUIRE(!third.is_in_back());

            REQUIRE(second.is_before(first));
            REQUIRE(third.is_before(first));
            REQUIRE(third.is_before(second));
            REQUIRE(!first.is_before(first));
            REQUIRE(!first.is_before(second));
            REQUIRE(!first.is_before(third));
            REQUIRE(!second.is_before(third));

            REQUIRE(first.is_behind(second));
            REQUIRE(first.is_behind(third));
            REQUIRE(second.is_behind(third));
            REQUIRE(!first.is_behind(first));
            REQUIRE(!second.is_behind(first));
            REQUIRE(!third.is_behind(first));
            REQUIRE(!third.is_behind(second));
        }
        SECTION("The z-order of Nodes can be modified") {
            SECTION("first.stack_front") {
                first.stack_front();
                REQUIRE(first.is_in_front());
                REQUIRE(second.is_in_back());
                REQUIRE(third.is_before(second));
                REQUIRE(third.is_behind(first));
            }
            SECTION("second.stack_front") {
                second.stack_front();
                REQUIRE(second.is_in_front());
                REQUIRE(first.is_in_back());
                REQUIRE(third.is_before(first));
                REQUIRE(third.is_behind(second));
            }
            SECTION("third.stack_front") {
                third.stack_front();
                REQUIRE(third.is_in_front());
                REQUIRE(first.is_in_back());
                REQUIRE(second.is_before(first));
                REQUIRE(second.is_behind(third));
            }

            SECTION("first.stack_back") {
                first.stack_back();
                REQUIRE(first.is_in_back());
                REQUIRE(second.is_before(first));
                REQUIRE(second.is_behind(third));
                REQUIRE(third.is_in_front());
            }
            SECTION("second.stack_back") {
                second.stack_back();
                REQUIRE(second.is_in_back());
                REQUIRE(first.is_before(second));
                REQUIRE(first.is_behind(third));
                REQUIRE(third.is_in_front());
            }
            SECTION("third.stack_back") {
                third.stack_back();
                REQUIRE(third.is_in_back());
                REQUIRE(first.is_before(third));
                REQUIRE(first.is_behind(second));
                REQUIRE(second.is_in_front());
            }

            SECTION("first.stack_before(first") {
                first.stack_before(first);
                REQUIRE(first.is_in_back());
            }
            SECTION("first.stack_before(second") {
                first.stack_before(second);
                REQUIRE(first.is_before(second));
                REQUIRE(first.is_behind(third));
            }
            SECTION("first.stack_before(third") {
                first.stack_before(third);
                REQUIRE(first.is_in_front());
            }

            SECTION("third.stack_behind(first)") {
                third.stack_behind(first);
                REQUIRE(third.is_in_back());
            }
            SECTION("third.stack_behind(second)") {
                third.stack_behind(second);
                REQUIRE(third.is_before(first));
                REQUIRE(third.is_behind(second));
            }
            SECTION("third.stack_behind(third)") {
                third.stack_behind(third);
                REQUIRE(third.is_in_front());
            }
        }
    }

    SECTION("Compile Time Nodes have Compile Time Properties") {
        auto node = root_node.create_child<LeafNodeCT>().to_owner();
        const int rt_value = node.get<int>("int");
        REQUIRE(node.get<int>("int"));

        SECTION("compile time and run time acquired Properties are the same") {
            const int ct_value = node.get(int_id);
            REQUIRE(rt_value == ct_value);

            static constexpr StringConst int_string_const = "int";
            REQUIRE(ct_value == node.get<int_string_const>());
        }

        SECTION("Compile Time Nodes can be asked for non-existing run time Properies") {
            REQUIRE_THROWS_AS(node.get<int>("float"), TypeError);
            REQUIRE_THROWS_AS(node.get<float>("int"), TypeError);
            REQUIRE_THROWS_AS(node.get<float>("not a property name"), NameError);
        }

        SECTION("you can change the property hash by changing any property value") {
            const size_t property_hash = Node::AccessFor<Tester>(node).get_property_hash();
            node.set(int_id, node.get(int_id) + 1);
            REQUIRE(property_hash != Node::AccessFor<Tester>(node).get_property_hash());
        }
    }

    SECTION("RunTimeNodes create their Properties in the constructor") {
        SECTION("Property names have to be unique") {
            struct NotUniquePropertyNode : public RunTimeNode {
                NOTF_UNUSED NotUniquePropertyNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {
                    _create_property<int>("not_unique", 0, true);
                    _create_property<int>("not_unique", 6587, false);
                }
            };
            REQUIRE_THROWS_AS(root_node.create_child<NotUniquePropertyNode>(), NotUniqueError);
        }

        SECTION("Properties can only be created in the constructor") {
            struct FinalizedNode : public RunTimeNode {
                NOTF_UNUSED FinalizedNode(valid_ptr<Node*> parent) : RunTimeNode(parent) {
                    _create_property<int>("int", 0, true);
                }

                void fail() {
                    NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
                    _create_property<int>("won't work because I'm finalized", 0, true);
                }
            };
            auto node_handle = root_node.create_child<FinalizedNode>().to_handle();
            auto node_ptr = to_shared_ptr(node_handle);
            REQUIRE_THROWS_AS(node_ptr->fail(), Node::FinalizedError);
        }
    }

    SECTION("Run Time Nodes have Run Time Properties") {
        NodeHandle node = root_node.create_child<LeafNodeRT>();

        SECTION("can be asked fo non-existing Properties") {
            REQUIRE(node.get<float>("float") != 0.0f);
            REQUIRE_THROWS_AS(node.get<float>("not a property"), NameError);
            REQUIRE_THROWS_AS(node.get<bool>("float"), TypeError);
        }

        SECTION("you can change the property hash by changing any property value") {
            const float before = node.get<float>("float");
            const size_t property_hash = Node::AccessFor<Tester>(node).get_property_hash();
            node.set("float", before + 1.f);
            REQUIRE(property_hash != Node::AccessFor<Tester>(node).get_property_hash());
        }
    }

    SECTION("whenever a Property changes, its Node is marked dirty") {
        TypedNodeHandle<LeafNodeCT> node_handle = root_node.create_child<LeafNodeCT>();
        auto node = std::dynamic_pointer_cast<LeafNodeCT>(
            TypedNodeHandle<LeafNodeCT>::AccessFor<Tester>::get_shared_ptr(node_handle));
        REQUIRE(node);

        REQUIRE(!Node::AccessFor<Tester>(*node.get()).is_dirty());
        node_handle.set(float_id, 223);
        REQUIRE(Node::AccessFor<Tester>(*node.get()).is_dirty());
    }

    SECTION("Nodes create a copy of their data to modify, if the graph is frozen") {
        auto node = root_node.create_child<LeafNodeCT>().to_handle();
        REQUIRE(node.get(int_id) == 123);
        {
            NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));
            node.set(int_id, 321);
            REQUIRE(node.get(int_id) == 321);
        }
        REQUIRE(node.get(int_id) == 321);
    }

    SECTION("Node Handles will not crash when you try call a method on them when they are expired") {
        NodeHandle expired;
        {
            auto owner = root_node.create_child<LeafNodeCT>().to_owner();
            expired = owner;
            REQUIRE(!expired.is_expired());
        }
        REQUIRE(expired.is_expired());

        REQUIRE_THROWS_AS(expired.stack_back(), HandleExpiredError); // mutable
        REQUIRE_THROWS_AS(expired.is_in_back(), HandleExpiredError); // const

        // TODO: test all node handle functions
    }

    SECTION("Two Nodes should have at least the root node as a common ancestor") {
        auto node = root_node.create_child<TestNode>().to_owner();
        auto first = to_shared_ptr(node)->create_child<TestNode>().to_owner();
        auto second = to_shared_ptr(node)->create_child<TestNode>().to_owner();
        auto third = to_shared_ptr(second)->create_child<TestNode>().to_owner();

        REQUIRE(first.get_common_ancestor(second) == node);
        REQUIRE(second.get_common_ancestor(first) == node);
        REQUIRE(first.get_common_ancestor(third) == node);
        REQUIRE(third.get_common_ancestor(first) == node);

        auto second_root = std::make_shared<TestNode>();
        NodeHandle foreign_node = second_root->create_child<LeafNodeCT>();
        REQUIRE_THROWS_AS(first.get_common_ancestor(foreign_node), Node::HierarchyError);

        REQUIRE(first.get_common_ancestor(first) == first);
    }
}

SCENARIO("Compile Time Nodes can be identified by type", "[app][node]") {
    REQUIRE(detail::is_compile_time_node_v<LeafNodeCT>);
    REQUIRE(!detail::is_compile_time_node_v<LeafNodeRT>);

    REQUIRE(detail::CompileTimeNodeIdentifier().test<LeafNodeCT>());
    REQUIRE(!detail::CompileTimeNodeIdentifier().test<LeafNodeRT>());
}
