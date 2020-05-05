#include "catch.hpp"

#include <thread>

#include "notf/meta/system.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("system", "[meta][system]") {
    SECTION("bitsizeof") {
        REQUIRE(bitsizeof<std::thread::id>() == 64);
    }
}
