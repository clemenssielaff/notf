#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_reactive_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/node.hpp"

NOTF_USING_NAMESPACE;

#ifdef NOTF_MSVC
static constexpr StringConst int_id_name = "int";
static constexpr StringConst bool_id_name = "bool";
static constexpr auto int_id = make_string_type<int_id_name>();
static constexpr auto bool_id = make_string_type<bool_id_name>();
#else
constexpr auto int_id = "int"_id;
constexpr auto bool_id = "bool"_id;
#endif

SCENARIO("Properties", "[app][property]") {
    //    // always reset the graph
    //    TheGraph::AccessFor<Tester>::reset();
    //    REQUIRE(TheGraph::AccessFor<Tester>::get_node_count() == 1);

    //    NodeHandle root_node_handle = TheGraph()->get_root_node();
    //    RootNodePtr root_node_ptr = std::static_pointer_cast<RootNode>(to_shared_ptr(root_node_handle));
    //    REQUIRE(root_node_ptr);
    //    auto root_node = Node::AccessFor<Tester>(*root_node_ptr);

    //    const auto render_thread_id = make_thread_id(78);

    //    SECTION("Properties have names") {
    //        auto node_rt = root_node.create_child<LeafNodeRT>().to_handle();
    //        auto node_ct = root_node.create_child<LeafNodeCT>().to_handle();

    //        REQUIRE(node_rt.get_property<int>("int").get_name() == "int");
    //        REQUIRE(node_ct.get_property<int>("int").get_name() == "int");
    //        REQUIRE(node_ct.get_property(int_id).get_name() == "int");
    //    }

    //    SECTION("Properties have default values") {
    //        auto node_rt = root_node.create_child<LeafNodeRT>().to_handle();
    //        auto node_ct = root_node.create_child<LeafNodeCT>().to_handle();

    //        REQUIRE(node_rt.get_property<int>("int").get_default() == 123);
    //        REQUIRE(node_ct.get_property<int>("int").get_default() == 123);
    //    }

    //    SECTION("Only changes in visible properties change a Node's property hash") {
    //        {
    //            auto node = root_node.create_child<LeafNodeCT>().to_owner();
    //            REQUIRE(!Node::AccessFor<Tester>(node).is_dirty());
    //            const size_t property_hash_before = Node::AccessFor<Tester>(node).get_property_hash();

    //            auto visible_property = node.get_property(int_id);
    //            REQUIRE(visible_property.is_visible());

    //            visible_property.set(node.get_property<int>("int").get_default());
    //            REQUIRE(!Node::AccessFor<Tester>(node).is_dirty());

    //            visible_property.set(node.get_property<int>("int").get_default() + 1);
    //            REQUIRE(Node::AccessFor<Tester>(node).is_dirty());

    //            const size_t property_hash_after = Node::AccessFor<Tester>(node).get_property_hash();
    //            REQUIRE(property_hash_before != property_hash_after);
    //        }
    //        {
    //            auto node = root_node.create_child<LeafNodeCT>().to_handle();
    //            REQUIRE(!Node::AccessFor<Tester>(node).is_dirty());
    //            const size_t property_hash_before = Node::AccessFor<Tester>(node).get_property_hash();

    //            auto invisible_property = node.get_property(bool_id);
    //            REQUIRE(!invisible_property.is_visible());

    //            invisible_property.set(!node.get_property<bool>("bool").get_default());
    //            REQUIRE(Node::AccessFor<Tester>(node).is_dirty());

    //            const size_t property_hash_after = Node::AccessFor<Tester>(node).get_property_hash();
    //            REQUIRE(property_hash_before != property_hash_after);
    //        }
    //    }

    //    SECTION("Properties can be used as reactive operators") {
    //        auto node = root_node.create_child<LeafNodeCT>().to_handle();
    //        auto property = node.get_property(int_id);

    //        auto publisher = TestPublisher();
    //        auto subscriber = TestSubscriber();

    //        auto pipeline = publisher | property | subscriber;

    //        SECTION("Property Operators tread new values like ones set by the user") {
    //            property.set(0);
    //            REQUIRE(property.get() == 0);

    //            publisher->publish(42);
    //            REQUIRE(property.get() == 42);

    //            REQUIRE(subscriber->values.size() == 2);
    //            REQUIRE(subscriber->values[0] == 0);
    //            REQUIRE(subscriber->values[1] == 42);
    //        }

    //        SECTION("Property Operators cannot be completed") {
    //            publisher->complete();
    //            REQUIRE(!subscriber->is_completed);
    //        }

    //        SECTION("Property Operators report but ignore all errors") {
    //            publisher->error(std::logic_error("That's illogical"));
    //            REQUIRE(!subscriber->is_completed);
    //            REQUIRE(subscriber->exception == nullptr);
    //        }
    //    }

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

    //    SECTION("Properties have optional callbacks") {
    //        auto node_ct = root_node.create_child<LeafNodeCT>().to_handle();
    //        auto property_ct = node_ct.get_property(int_id);
    //        auto property_ct_ptr = to_shared_ptr(property_ct);

    //        property_ct.set(18);
    //        REQUIRE(property_ct.get() == 18);

    //        property_ct_ptr->set_callback([](int& value) {
    //            if (value > 10) {
    //                value += 2;
    //                return true;
    //            } else {
    //                return false;
    //            }
    //        });

    //        property_ct.set(40); // is accepted by the callback
    //        REQUIRE(property_ct.get() == 42);

    //        property_ct.set(8); // is rejected by the callback
    //        REQUIRE(property_ct.get() == 42);
    //    }

    //    SECTION("Property Handles can be compared") {
    //        auto node1 = root_node.create_child<LeafNodeCT>().to_owner();
    //        auto handle1 = node1.get_property(int_id);

    //        auto node2 = root_node.create_child<LeafNodeCT>().to_owner();
    //        auto handle2 = node2.get_property(int_id);

    //        REQUIRE(handle1 == handle1);
    //        REQUIRE(handle1 != handle2);
    //        REQUIRE((handle1 < handle2 || handle2 < handle1));
    //    }

    //    SECTION("Property Handles can expire") {
    //        PropertyHandle<int> handle;
    //        REQUIRE(!handle);

    //        SECTION("Property Handles expire with their Node") {
    //            {
    //                auto node = root_node.create_child<LeafNodeRT>().to_owner();
    //                handle = node.get_property<int>("int");
    //                REQUIRE(handle);
    //            }
    //            REQUIRE(handle.is_expired());

    //            REQUIRE_THROWS_AS(handle.get(), HandleExpiredError);
    //            REQUIRE_THROWS_AS(handle.set(78), HandleExpiredError);
    //        }

    //        SECTION("Expired handles will throw when used as reactive Operators") {
    //            auto publisher = TestPublisher();
    //            auto subscriber = TestSubscriber();

    //            REQUIRE_THROWS_AS(publisher | handle, HandleExpiredError);
    //            REQUIRE_THROWS_AS(handle | subscriber, HandleExpiredError);
    //            REQUIRE_THROWS_AS(handle | handle, HandleExpiredError);
    //        }
    //    }
}
