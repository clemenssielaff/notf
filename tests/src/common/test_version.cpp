#include "catch2/catch.hpp"

#include "notf/common/version.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("version", "[common][version]") {
    SECTION("versions can be compared") {
        REQUIRE(Version(45) == Version(45));
        REQUIRE(Version(45) != Version(40));
        REQUIRE(Version(45) >= Version(45));
        REQUIRE(Version(45) > Version(44));
        REQUIRE(Version(45) <= Version(45));
        REQUIRE(Version(45) < Version(46));
    }

    SECTION("test current version") { REQUIRE(get_notf_version() > Version(0)); }
}
