#include "catch2/catch.hpp"

#include "test_app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("run time node", "[app][node]")
{
//    NOTF_USING_LITERALS;
//    auto node = std::make_shared<CompileTimeTestNode>();

//    SECTION("compile time property")
//    {
//        auto ct_handle = node->get_property("float"_id);
//        REQUIRE(ct_handle.get_name() == "float");
//        REQUIRE(is_approx(ct_handle.get(), 0.123f));

//        auto rt_handle = node->get_property<bool>("bool");
//        REQUIRE(rt_handle.get_name() == "bool");
//        REQUIRE(rt_handle.get());

//        REQUIRE(node->get_property("bool"_id) == rt_handle);
//    }

//    SECTION("compile time property hash")
//    {
//        auto accessor = Node::AccessFor<Tester>(*node.get());
//        const size_t default_hash = accessor.get_property_hash();
//        node->get_property("float"_id).set(0);
//        REQUIRE(default_hash != accessor.get_property_hash());
//    }
}
