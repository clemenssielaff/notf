#include "catch2/catch.hpp"

#include "notf/meta/debug.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("debug", "[meta][debug]")
{
    SECTION("remove all occurrences of 'element' from a vector") { REQUIRE(is_debug_build()); }

    SECTION("read the file name from the __FILE__ macro")
    {
        const std::string this_file_name = filename_from_path(__FILE__);
        REQUIRE(this_file_name == "test_debug.cpp");
    }
}
