#include "catch.hpp"

#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("ConstStrings", "[meta][stringtype]") {
    constexpr ConstString test_string = "th/s_1s-A:T3st! 0";
    size_t constexpr_hash = test_string.get_hash();

    REQUIRE(test_string.get_size() == 17);
    REQUIRE(test_string.get_view() == std::string_view("th/s_1s-A:T3st! 0"));
    REQUIRE(constexpr_hash == hash_string(test_string.c_str(), test_string.get_size()));

    constexpr ConstString other_string = "th/s_1s-A:T3st! 0";
    constexpr ConstString wrong_string1 = "too short";
    constexpr ConstString wrong_string2 = "th/s_1s-A:T3st! 1";
    REQUIRE(test_string == other_string);
    REQUIRE(test_string != wrong_string1);
    REQUIRE(test_string != wrong_string2);

#ifndef NOTF_MSVC
    SECTION("id literal") {
        using namespace notf::literals;
        constexpr ConstString derbe_id = "derbe";
        REQUIRE("derbe"_id == derbe_id);
        REQUIRE(!("underbe"_id == derbe_id));
        REQUIRE(!("darbe"_id == derbe_id));
        REQUIRE(derbe_id[2] == 'r');
        REQUIRE("underbe"_id.at(2) == 'd');
        REQUIRE_THROWS_AS(derbe_id[500], std::logic_error);

        REQUIRE("derbe"_id.at(2) == 'r');
        REQUIRE_THROWS_AS("derbe"_id.at(40), std::logic_error);
    }
#endif
}
