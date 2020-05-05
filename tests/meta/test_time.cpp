#include "catch.hpp"

#include "notf/meta/real.hpp"
#include "notf/meta/time.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("time", "[meta][time]") {
    SECTION("fps literal") {
        using namespace notf::literals;

        // not const, otherwise this is not executed at runtime
        auto fps24_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(24_fps).count();
        REQUIRE(fps24_in_ms == 1000 / 24);

        auto fps60_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(60._fps).count();
        REQUIRE(fps60_in_ms == 1000 / 60);
    }

    SECTION("age and now is ever increasing") {
        const auto the_past = get_now();
        const auto original_age = get_age();
        REQUIRE(the_past < get_now());
        REQUIRE(original_age < get_age());
    }
}
