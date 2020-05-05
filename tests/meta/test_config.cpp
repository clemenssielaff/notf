#include "catch.hpp"

#include "notf/meta/config.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("config", "[meta][config]") {
    SECTION("just for coverage") {
        // force some functions to execute at compile time
        const std::string version_string = config::version_string();
        const std::string compiler_name = config::compiler_name();

        REQUIRE(config::version_major() == 0);
        REQUIRE(config::version_minor() == 5);
        REQUIRE(config::version_patch() == 0);
        REQUIRE(version_string == "0.5.0");
        REQUIRE(compiler_name == "Clang");
        REQUIRE(config::is_big_endian() == 0);
        REQUIRE(config::is_debug_build() == true);
        REQUIRE(config::abort_on_assert() == false);
    }
}
