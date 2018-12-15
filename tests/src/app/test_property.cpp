#include "catch2/catch.hpp"

#include "test/app.hpp"
#include "test/reactive.hpp"

#include "notf/app/graph/property.hpp"

NOTF_USING_NAMESPACE;

#ifdef NOTF_MSVC
static constexpr ConstString int_id_name = "int";
static constexpr ConstString bool_id_name = "bool";
static constexpr auto int_id = make_string_type<int_id_name>();
static constexpr auto bool_id = make_string_type<bool_id_name>();
#else
constexpr auto int_id = "int"_id;
constexpr auto bool_id = "bool"_id;
#endif

SCENARIO("Properties", "[app][property]") {
    TheApplication app(TheApplication::Arguments{});
    auto root_node = TheRootNode();
    auto root_node_handle = TheGraph()->get_root_node();

    SECTION("Properties have names") {
        auto node_rt = TheRootNode().create_child<TestNode>().to_handle();
        auto node_ct = TheRootNode().create_child<TestNode>().to_handle();

        REQUIRE(node_rt->get<int>("int") == 123);
        REQUIRE(node_ct->get<int>("int") == 123);
    }

    SECTION("Only changes in visible properties change a Node's property hash") {
        { // visible property
            auto node = root_node.create_child<TestNode>().to_owner();
            REQUIRE(!node->is_dirty());
            const size_t property_hash_before = AnyNode::AccessFor<Tester>(node).get_property_hash();

            node->set(int_id, 123); // default value
            REQUIRE(!node->is_dirty());

            node->set(int_id, 999);
            REQUIRE(node->is_dirty());

            const size_t property_hash_after = AnyNode::AccessFor<Tester>(node).get_property_hash();
            REQUIRE(property_hash_before != property_hash_after);
        }
        { // invisible property
            auto node = root_node.create_child<TestNode>().to_handle();
            REQUIRE(!node->is_dirty());
            const size_t property_hash_before = AnyNode::AccessFor<Tester>(node).get_property_hash();

            node->set(bool_id, true); // default value
            REQUIRE(!node->is_dirty());

            node->set(bool_id, false);
            REQUIRE(!node->is_dirty());

            const size_t property_hash_after = AnyNode::AccessFor<Tester>(node).get_property_hash();
            REQUIRE(property_hash_before != property_hash_after); // property hash still differs
        }
    }

    SECTION("Properties can be used as reactive operators") {
        auto node = root_node.create_child<TestNode>().to_handle();

        auto publisher = TestPublisher();
        auto subscriber = TestSubscriber();

        auto pipeline = publisher | node->connect_property<int>("int") | subscriber;

        SECTION("Property Operators tread new values like ones set by the user") {
            node->set("int", 0);
            REQUIRE(node->get<int>("int") == 0);

            node->set("int", 42);
            REQUIRE(node->get<int>("int") == 42);

            REQUIRE(subscriber->values.size() == 2);
            REQUIRE(subscriber->values[0] == 0);
            REQUIRE(subscriber->values[1] == 42);
        }

        SECTION("Property Operators cannot be completed") {
            publisher->complete();
            REQUIRE(!subscriber->is_completed);
        }

        SECTION("Property Operators report but ignore all errors") {
            publisher->error(std::logic_error("That's illogical"));
            REQUIRE(!subscriber->is_completed);
            REQUIRE(subscriber->exception == nullptr);
        }
    }

    //    SECTION("Properties are frozen with the Graph") {
    //        auto node_ct = root_node.create_child<LeafNodeCT>().to_handle();
    //        auto property_ct = node_ct.get_property(int_id);
    //        auto property_ct_ptr = to_shared_ptr(property_ct);

    //        auto node_rt = root_node.create_child<LeafNodeRT>().to_handle();
    //        auto property_rt = node_rt.get_property<int>("int");
    //        auto property_rt_ptr = to_shared_ptr(property_rt);

    //        property_ct.set(0);
    //        property_rt.set(0);
    //        REQUIRE(property_ct.get() == 0);
    //        REQUIRE(property_rt.get() == 0);
    //        {
    //            NOTF_GUARD(TheGraph::AccessFor<Tester>::freeze(render_thread_id));

    //            property_ct.set(43);
    //            REQUIRE(property_ct_ptr->get() == 43);
    //            REQUIRE(property_ct_ptr->get(render_thread_id) == 0);

    //            property_rt.set(43);
    //            REQUIRE(property_rt_ptr->get() == 43);
    //            REQUIRE(property_rt_ptr->get(render_thread_id) == 0);

    //            property_ct.set(835);
    //            REQUIRE(property_ct_ptr->get() == 835);
    //            REQUIRE(property_ct_ptr->get(render_thread_id) == 0);

    //            property_rt.set(835);
    //            REQUIRE(property_rt_ptr->get() == 835);
    //            REQUIRE(property_rt_ptr->get(render_thread_id) == 0);
    //        }
    //        REQUIRE(property_ct.get() == 835);
    //        REQUIRE(property_rt.get() == 835);
    //    }

    SECTION("Properties have optional callbacks") {
        struct CallbackNode : public TestNode {
            NOTF_UNUSED CallbackNode(valid_ptr<AnyNode*> parent) : TestNode(parent) {
                set("int", 18);
                _set_property_callback<int>("int", [](int& value) {
                    if (value > 10) {
                        value += 2;
                        return true;
                    } else {
                        return false;
                    }
                });
            }
        };
        auto node = root_node.create_child<CallbackNode>().to_handle();
        REQUIRE(node->get<int>("int") == 18);

        node->set("int", 40); // is accepted by the callback
        REQUIRE(node->get<int>("int") == 42);

        node->set("int", 8); // is accepted by the callback
        REQUIRE(node->get<int>("int") == 42);
    }
}
