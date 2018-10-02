#include "catch2/catch.hpp"

#include "notf/common/string_view.hpp"
#include "notf/meta/stringtype.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("string_view", "[common][string_view]")
{
    SECTION("hash a string_view")
    {
        constexpr StringConst test_string = "th/s_1s-A:T3st! 0";
        size_t constexpr_hash = test_string.get_hash();
        std::string_view string_view(test_string.c_str());

        REQUIRE(constexpr_hash == hash_string(string_view));
    }
}
