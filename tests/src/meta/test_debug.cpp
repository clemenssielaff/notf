#include "catch.hpp"

#include "notf/meta/debug.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("debug", "[meta][debug]") {
    SECTION("tests are always built in debug mode") { REQUIRE(config::is_debug_build()); }

    SECTION("read the file name from the __FILE__ macro") {
        const std::string this_file_name = filename_from_path(__FILE__);
        REQUIRE(this_file_name == "test_debug.cpp");
    }
}
