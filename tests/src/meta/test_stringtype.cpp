#include "catch2/catch.hpp"

#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("StringConsts", "[meta][stringtype]") {
    constexpr StringConst test_string = "th/s_1s-A:T3st! 0";
    size_t constexpr_hash = test_string.get_hash();

    REQUIRE(test_string.get_size() == 17);
    REQUIRE(constexpr_hash == hash_string(test_string.c_str(), test_string.get_size()));

#ifndef NOTF_MSVC
    SECTION("id literal") {
        using namespace notf::literals;
        constexpr StringConst derbe_id = "derbe";
        REQUIRE("derbe"_id == derbe_id);
        REQUIRE(!("underbe"_id == derbe_id));
        REQUIRE(!("darbe"_id == derbe_id));
        REQUIRE(derbe_id[2] == 'r');
        REQUIRE_THROWS_AS(derbe_id[500], std::logic_error);

        REQUIRE("derbe"_id.at(2) == 'r');
        REQUIRE_THROWS_AS("derbe"_id.at(40), std::logic_error);
    }
#endif
}
