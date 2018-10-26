#include "catch2/catch.hpp"

#include "test_app_utils.hpp"
#include "test_utils.hpp"

#include "notf/app/app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("Compile Time Nodes can be identified by type", "[app]")
{
    REQUIRE(detail::is_compile_time_node_v<LeafNodeCT>);
    REQUIRE(!detail::is_compile_time_node_v<LeafNodeRT>);

    REQUIRE(detail::CompileTimeNodeIdentifier().test<LeafNodeCT>());
    REQUIRE(!detail::CompileTimeNodeIdentifier().test<LeafNodeRT>());
}
